#ifndef _NRF_RADIO_DRIVER_H
#define _NRF_RADIO_DRIVER_H

typedef enum{
	MODE_TX, MODE_RX
}radio_mode_t;

typedef  void (*radio_event_handler_t)(void);

typedef enum{

	RADIO_1MBPS, RADIO_2MBPS
}radio_data_rate_t;

typedef enum{
	RADIO_LITTLE_ENDIAN, RADIO_BIG_ENDIAN
}radio_endian_t;

typedef enum
{
	DISABLED, RX_RU, RX_IDLE, RX, RX_DISABLE, TX_RU=9, TX_IDLE, TX, TX_DISABLE
}radio_state_t;


void radio_set_frequency(uint32_t frequency);

void radio_set_tx_power(uint32_t tx_power);

void radio_set_data_rate(radio_data_rate_t data_rate);

void radio_disable();

void radio_set_tx_address(const uint8_t* address, uint8_t logical_address);

void radio_set_rx_address(const uint8_t* address, uint8_t logical_address);

uint8_t radio_get_crc_status();

void radio_set_address_width(uint8_t add_width);

void radio_set_whiteiv(uint8_t init_value);

void radio_set_mode(radio_mode_t mode);

void radio_set_payload_endian(radio_endian_t endian);

void radio_configure_crc(uint8_t crc_len, uint8_t crc_add, uint32_t crc_poly, uint32_t crc_init_val);

void radio_start_tx();

void radio_tx();

void radio_start_rx();

void radio_enable_interrupts();

void radio_config_pl_size(uint8_t max_pln, bool dynamic_pl);

void radio_enable_whitening(bool en);

void radio_set_packet_ptr(uint8_t* ptr);

void radio_set_evt_handler(radio_event_handler_t evt_handler);

uint8_t radio_get_received_address();

radio_state_t radio_get_state();

#endif