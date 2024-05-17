#ifndef NRF24_H
#define NRF24_H
#define NRF24_TX_FIFO_MAX_SIZE 1024
#define NRF24_ESB
typedef struct
{
#if defined(NRF24_ESB)
	uint8_t PID;
	uint8_t No_ack;

#endif
	uint8_t payload[32];
} nrf24_packet_t;

typedef enum
{
	NRF24_TX_SUCCESS,
	NRF24_TX_FAILED,
	NRF24_DATA_READY,
	NRF24_ACK_SENT,
	NRF24_INVALID_OPERATION
} nrf24_evt_type_t;

typedef struct
{
	nrf24_evt_type_t event_type;
	uint8_t *data;
} nrf24_event_t;

typedef enum
{
	NRF24_MODE_TX,
	NRF24_MODE_RX
} nrf24_mode_t;

typedef struct
{
	nrf24_packet_t tx_fifo_buffer[NRF24_TX_FIFO_MAX_SIZE];
	uint8_t write_index;
	uint8_t read_index;
	uint8_t count;
} nrf24_tx_fifo_t;

typedef void (*nrf24_evt_handler_t)(nrf24_event_t *nrf24_evt);

void nrf24_set_tx_address(const uint8_t *tx_address);

void nrf24_set_rx_address(const uint8_t *rx_address, uint8_t pipe);

void nrf24_send(uint8_t *message, uint8_t length);

void nrf24_set_evt_handler(nrf24_evt_handler_t evt_handler);

void nrf24_set_mode(nrf24_mode_t mode);

void nrf24_init();

#endif
