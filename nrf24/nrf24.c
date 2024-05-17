#include <stdint.h>
#include <string.h>

#include "boards.h"
#include "nrf24.h"
#include "radio_driver.h"
#include "debug_log.h"
#include "softTimer.h"

#if defined(NRF24_ESB)
#define AUTO_RETRANSMIT_DELAY US_TO_TICKS(1000)
#define MAX_RETRIES 15
SOFT_TIMER_DEF(auto_retransmit_timer);
volatile uint8_t auto_retransmit_count = 0;
#endif

nrf24_evt_handler_t nrf24_event_handler;
nrf24_packet_t recv_ack_packet;

nrf24_packet_t tx_packet = {
#if defined(NRF24_ESB)
	.PID = 0x84, .No_ack = 0
#endif
};

nrf24_packet_t *p_current_packet;

nrf24_tx_fifo_t tx_fifo;

nrf24_event_t nrf24_event;

nrf24_mode_t nrf24_mode;
uint8_t pid = 0;

nrf24_packet_t recv_packet;
uint8_t prev_pid = 4;

void nrf24_tx_fifo_execute(); // Forward declaration

// Function to check if the Queue is full
bool tx_fifo_full()
{
	if (tx_fifo.count == NRF24_TX_FIFO_MAX_SIZE)
		return 1;
	return 0;
}

// Function to check if the Queue is empty
bool tx_fifo_empty()
{
	if (tx_fifo.count == 0)
		return 1;
	return 0;
}

// Function to insert element into the Queue
void tx_fifo_push(nrf24_packet_t packet)
{
	if (!tx_fifo_full())
	{
		tx_fifo.tx_fifo_buffer[tx_fifo.write_index] = packet;
		tx_fifo.write_index = (tx_fifo.write_index + 1) % NRF24_TX_FIFO_MAX_SIZE;
		tx_fifo.count++;
	}
	else
		debug_log_print("TX fifo is full\n");
}

void nrf24_set_evt_handler(nrf24_evt_handler_t evt_handler)
{
	nrf24_event_handler = evt_handler;
}

static uint8_t reverse_bit_order(uint8_t byte)
{
	uint8_t temp = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		if (byte & 1)
			temp |= 0x80 >> i;
		byte >>= 1;
	}
	return temp;
}

static void nrf24_set_pl_size(uint8_t pl_size)
{
#if defined(NRF24_ESB)
	radio_set_s0_field_size(1);				// 1byte->6bits len+2bits pid
	radio_set_s1_field_size(1);				// 1bit->no_ack flag
	radio_set_static_payload_size(pl_size); // static payload len
	radio_set_max_payload_size(pl_size);	// max payload len
#else
	radio_set_static_payload_size(pl_size);
	radio_set_max_payload_size(pl_size);
#endif
}

void nrf24_set_tx_address(const uint8_t *tx_address)
{
	uint8_t tx_addr_rev[5] = {0};
	for (uint8_t i = 0; i < 5; i++)
		tx_addr_rev[i] = reverse_bit_order(tx_address[i]);

	radio_set_tx_address(tx_addr_rev, 0);
}

void nrf24_set_rx_address(const uint8_t *rx_address, uint8_t pipe)
{

	uint8_t rx_addr_rev[5] = {0};
	for (uint8_t i = 0; i < 5; i++)
		rx_addr_rev[i] = reverse_bit_order(rx_address[i]);

	radio_set_rx_address(rx_addr_rev, pipe);
}

void nrf24_set_mode(nrf24_mode_t mode)
{
	nrf24_mode = mode;
	switch (mode)
	{
	case NRF24_MODE_RX:
		radio_set_packet_ptr((uint8_t *)&recv_packet);
		radio_set_mode(MODE_RX);
		radio_start_rx();
		break;
	case NRF24_MODE_TX:
#if defined(NRF24_ESB)
		radio_set_rx_logical_address(0); // for receiving ack packets
#else
		radio_set_mode(MODE_TX);
#endif
		break;
	default:
		break;
	}
}

#if defined(NRF24_ESB)

void nrf24_tx_and_wait_for_ack()
{

	nrf24_set_pl_size(32);
	radio_set_packet_ptr((uint8_t *)p_current_packet);
	radio_set_mode(MODE_TX);
	radio_start_tx();

	nrf24_set_pl_size(0);
	radio_set_packet_ptr((uint8_t *)&recv_ack_packet);
	radio_set_mode(MODE_RX);
	radio_start_rx();
}

void auto_retransmit_handler()
{

	if (auto_retransmit_count >= MAX_RETRIES)
	{
		softTimer_stop(&auto_retransmit_timer);
		auto_retransmit_count = 0;
		nrf24_event.event_type = NRF24_TX_FAILED;
		nrf24_event.data = NULL;
		nrf24_event_handler(&nrf24_event);
		tx_fifo.count--;
		NRF_EGU1->TASKS_TRIGGER[0] = 1;
		return;
	}
	nrf24_tx_and_wait_for_ack();
	auto_retransmit_count++;
}

#endif

void nrf24_tx_fifo_execute()
{
	p_current_packet = &tx_fifo.tx_fifo_buffer[tx_fifo.read_index];
	tx_fifo.read_index = (tx_fifo.read_index + 1) % NRF24_TX_FIFO_MAX_SIZE;

#if defined(NRF24_ESB)
	nrf24_tx_and_wait_for_ack();
	softTimer_start(&auto_retransmit_timer, AUTO_RETRANSMIT_DELAY);

#else

	radio_set_packet_ptr((uint8_t *)p_current_packet);
	radio_start_tx();

	nrf24_event.event_type = NRF24_TX_SUCCESS;
	nrf24_event.data = NULL;
	nrf24_event_handler(&nrf24_event);
	tx_fifo.count--;
	NRF_EGU1->TASKS_TRIGGER[0] = 1;
#endif
}

void nrf24_send(uint8_t *nrf24_tx_msg, uint8_t length)
{

	if (nrf24_mode == NRF24_MODE_TX)
	{
		memset(tx_packet.payload, 0, sizeof(tx_packet.payload));
#if defined(NRF24_ESB)
		tx_packet.PID &= 0xFC;
		tx_packet.PID |= pid;
		pid++;
		if (pid > 3)
			pid = 0;
#endif
		memcpy(tx_packet.payload, nrf24_tx_msg, length);

		if (tx_fifo_empty())
		{
			tx_fifo_push(tx_packet);
			NRF_EGU1->TASKS_TRIGGER[0] = 1;
		}
		else
			tx_fifo_push(tx_packet);
		if (tx_fifo_full())
			NRF_EGU1->TASKS_TRIGGER[0] = 1;
	}
	else
	{
		nrf24_event.event_type = NRF24_INVALID_OPERATION;
		nrf24_event.data = NULL;
		nrf24_event_handler(&nrf24_event);
	}
}

void nrf24_handle_packet()
{
	switch (nrf24_mode)
	{
	case NRF24_MODE_RX:
#if defined(NRF24_ESB)
		if (radio_get_received_address() == 1)
		{
			radio_set_tx_logical_address(1);
			nrf24_set_pl_size(0);
			radio_set_mode(MODE_TX);
			radio_start_tx();
			if ((recv_packet.PID & 0x03) != prev_pid)
			{
				prev_pid = recv_packet.PID & 0x03;
				nrf24_event.event_type = NRF24_DATA_READY;
				nrf24_event.data = recv_packet.payload;
				nrf24_event_handler(&nrf24_event);
			}

			memset(&recv_packet, 0, sizeof(recv_packet));
			nrf24_set_pl_size(32);
			radio_set_mode(MODE_RX);
			radio_start_rx();
		}
#else
		if (radio_get_received_address() == 1)
		{
			nrf24_event.event_type = NRF24_DATA_READY;
			nrf24_event.data = recv_packet.payload;
			nrf24_event_handler(&nrf24_event);
			memset(&recv_packet, 0, sizeof(recv_packet));
			radio_start_rx();
		}
#endif

		break;

	case NRF24_MODE_TX:

#if defined(NRF24_ESB)
		if (radio_get_received_address() == 0)
		{
			softTimer_stop(&auto_retransmit_timer);
			auto_retransmit_count = 0;
			nrf24_event.event_type = NRF24_TX_SUCCESS;
			nrf24_event.data = NULL;
			nrf24_event_handler(&nrf24_event);
			tx_fifo.count--; // decrement the count value after the TX has completed
			NRF_EGU1->TASKS_TRIGGER[0] = 1;
		}
#endif

		break;
	default:
		break;
	}
}

void radio_evnt_handler()
{

	nrf24_handle_packet();
}

void egu1_init()
{
	NRF_EGU1->INTENSET = 1UL;
	NVIC_EnableIRQ(SWI1_EGU1_IRQn);
}

void nrf24_init()
{
	egu1_init();
	uint32_t nrf24_crc_poly = 0x11021UL;
	radio_set_payload_endian(RADIO_BIG_ENDIAN);
	radio_configure_crc(2, 0, nrf24_crc_poly, 0xFFFF);
	radio_enable_whitening(0);
#if defined(NRF24_ESB)
	softTimer_create(&auto_retransmit_timer, auto_retransmit_handler, SOFT_TIMER_MODE_REPEATED);
#else
	nrf24_set_pl_size(32);
#endif
	radio_set_data_rate(RADIO_1MBPS);
	radio_set_frequency(76);
	radio_set_address_width(5);
	radio_set_tx_power(0x00);
	radio_set_evt_handler(radio_evnt_handler);
	radio_enable_interrupts();
}

void egu1_handler()
{
	if (!tx_fifo_empty())
	{
		nrf24_tx_fifo_execute();
	}
}

void SWI1_EGU1_IRQHandler()
{

	if (NRF_EGU1->EVENTS_TRIGGERED[0])
	{
		NRF_EGU1->EVENTS_TRIGGERED[0] = 0;
		egu1_handler();
	}
}
