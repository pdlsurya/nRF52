#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "nrf52_ble.h"
#include "radio_driver.h"
#include "softTimer.h"
#include "debug_log.h"
#include "bsp.h"

#define ADV_INTERVAL MS_TO_TICKS(100)
#define SCAN_INTERVAL MS_TO_TICKS(300)
SOFT_TIMER_DEF(adv_timer);
SOFT_TIMER_DEF(scan_timer);

const uint8_t channel[3] = {37, 38, 39};  // logical BLE channel number (37-39)
const uint8_t frequency[3] = {2, 26, 80}; // physical frequency (2400+x MHz)
const uint8_t adv_addr[4] = {0x8E, 0x89, 0xBE, 0xD6};
uint8_t srv_data_cnt;
ble_instance_t ble_instance;
uint8_t pl_size;
uint32_t ble_crc_poly = 0x100065BUL;
uint32_t crc_init = 0x555555UL;
// uint32_t crc_poly=0x11021UL;

static void ble_set_pl_size(uint8_t pl_size)
{
	radio_set_len_field_size(8);
	radio_set_s0_field_size(1);
	radio_set_max_payload_size(pl_size);
}

void add_adv_data(uint8_t data_size, uint8_t type, uint8_t *data)
{

	ble_adv_data_t *ptr = (ble_adv_data_t *)(ble_instance.adv_pdu_buffer.payload + ble_instance.pl_size);
	ptr->length = data_size + 1;
	ptr->type = type;
	for (uint8_t i = 0; i < data_size; i++)
		ptr->data[i] = data[i];

	ble_instance.pl_size += data_size + 2;
}

void hop_channel()
{
	ble_instance.current_freq_index++;
	if (ble_instance.current_freq_index >= sizeof(channel))
		ble_instance.current_freq_index = 0;
	radio_set_frequency(frequency[ble_instance.current_freq_index]);
}

void update_service_data(ble_service_data_t *service_data)
{

	ble_instance.config_data->ble_service_data = service_data;
}

void assemble_pdu()
{
	ble_instance.pl_size = 0;

	// add adv flasgs
	add_adv_data(1, 0x01, &ble_instance.config_data->flags);

	// add complete local name
	add_adv_data(strlen(ble_instance.config_data->adv_name), 0x09,
				 (uint8_t *)ble_instance.config_data->adv_name);

	// add service data
	uint8_t *service_data = (uint8_t *)ble_instance.config_data->ble_service_data;
	add_adv_data(sizeof(ble_service_data_t), 0x16, service_data);

	ble_instance.adv_pdu_buffer.mac_address[0] = 0xA7;
	ble_instance.adv_pdu_buffer.mac_address[1] = 0x05;
	ble_instance.adv_pdu_buffer.mac_address[2] = 0xD5;
	ble_instance.adv_pdu_buffer.mac_address[3] = 0x7C;
	ble_instance.adv_pdu_buffer.mac_address[4] = 0xBB;
	ble_instance.adv_pdu_buffer.mac_address[5] = 0xFB;
	ble_instance.pl_size += 6;
	ble_instance.adv_pdu_buffer.header = 0x22;
	ble_instance.adv_pdu_buffer.payload_length = ble_instance.pl_size;
	ble_set_pl_size(ble_instance.pl_size + 2);
}

void radio_evt_handler()
{
	ble_instance.scan_event_handler(&ble_instance.scan_pdu_buffer);
}

void set_ble_mode(ble_mode_t mode)
{
	switch (mode)
	{
	case BLE_MODE_ADV:
		radio_set_tx_address(adv_addr, 0);
		radio_set_packet_ptr((uint8_t *)&ble_instance.adv_pdu_buffer);
		radio_set_mode(MODE_TX);
		break;

	case BLE_MODE_SCAN:
		radio_set_rx_address(adv_addr, 1);
		ble_set_pl_size(72);
		radio_set_packet_ptr((uint8_t *)&ble_instance.scan_pdu_buffer);
		radio_set_mode(MODE_RX);
	}
}

void ble_advertise()
{
	__disable_irq();
	ble_instance.config_data->ble_service_data->data = srv_data_cnt;
	srv_data_cnt++;
	assemble_pdu();
	radio_start_tx();

	hop_channel();
	radio_set_whiteiv(channel[ble_instance.current_freq_index]);
	// bsp_board_led_invert(3);
	__enable_irq();
}

void adv_timer_handler()
{
	ble_advertise();
}

void ble_begin(ble_config_t *config_data, scan_event_handler_t evt_handler)
{
	if (config_data != NULL)
	{
		ble_instance.config_data = config_data;
		ble_instance.current_freq_index = 0;
	}
	ble_instance.scan_event_handler = evt_handler;
	radio_set_evt_handler(radio_evt_handler);
	radio_set_address_width(4);
	radio_set_tx_power(0x00);
	radio_enable_whitening(1);
	radio_set_data_rate(RADIO_1MBPS);
	radio_set_frequency(frequency[ble_instance.current_freq_index]);
	radio_configure_crc(3, 1, ble_crc_poly, crc_init);
	radio_set_whiteiv(channel[ble_instance.current_freq_index]);
	radio_set_payload_endian(RADIO_LITTLE_ENDIAN);
	radio_enable_interrupts();
	softTimer_create(&adv_timer, adv_timer_handler,
					 SOFT_TIMER_MODE_REPEATED);
}

void scan_timer_handler()
{

	radio_start_rx();
	hop_channel();
	radio_set_whiteiv(channel[ble_instance.current_freq_index]);
	bsp_board_led_invert(2);
}

void ble_start_advertising()
{

	softTimer_start(&adv_timer, ADV_INTERVAL);
}

void ble_stop_advertising()
{

	softTimer_stop(&adv_timer);
	bsp_board_led_off(3);
}

void ble_start_scanning()
{
	softTimer_create(&scan_timer, scan_timer_handler,
					 SOFT_TIMER_MODE_REPEATED);
	softTimer_start(&scan_timer, SCAN_INTERVAL);
}
