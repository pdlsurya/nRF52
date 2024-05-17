#ifndef _NRF52_BLE
#define _NRF52_BLE

#include "stdint.h"
#include "stdbool.h"

#define NRF_TEMPERATURE_SERVICE_UUID		0x1809
#define NRF_BATTERY_SERVICE_UUID			0x180F
#define NRF_DEVICE_INFORMATION_SERVICE_UUID 0x180A
#define NRF_HEART_RATE_SERVICE_UUID         0X180D


typedef struct 
{
	uint8_t header;
	uint8_t payload_length;
	uint8_t mac_address[6];
	uint8_t payload[64];

}ble_adv_pdu_t;

typedef  void (*scan_event_handler_t)(ble_adv_pdu_t* pdu_buffer);

typedef struct 
{
	uint8_t length;
	uint8_t type;
	uint8_t data[];
}ble_adv_data_t;

typedef struct 
{
	uint16_t uuid;
	uint8_t data;

}ble_service_data_t;

typedef struct 
{
	char* adv_name;
	uint8_t flags;
	ble_service_data_t  *ble_service_data;
}ble_config_t;

typedef enum{
	BLE_MODE_ADV, BLE_MODE_SCAN
}ble_mode_t;

typedef struct 
{
	uint8_t current_freq_index;
	ble_adv_pdu_t adv_pdu_buffer;
	ble_adv_pdu_t scan_pdu_buffer;
	uint8_t pl_size;
	ble_config_t* config_data;
	scan_event_handler_t scan_event_handler;
}ble_instance_t;

void set_ble_mode(ble_mode_t mode);
void ble_start_advertising();
void ble_advertise();
void ble_start_scanning();
void ble_stop_advertising();
void update_service_data(ble_service_data_t* service_data);
void ble_begin(ble_config_t* config, scan_event_handler_t evt_handler);
#endif
