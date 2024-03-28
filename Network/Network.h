#ifndef _NETWORK_H_
#define _NETWORK_H_

typedef void(*nw_evt_handler_t)(uint8_t* data, uint8_t length);

typedef enum{
	NW_MSG_DATA, NW_MSG_ACK, NW_MSG_PING, NW_MSG_PING_ACK
}nw_msg_type_t;

typedef struct {
	uint16_t to_node;
	uint16_t from_node;
	nw_msg_type_t msg_type;

} network_header_t;

typedef struct {
	network_header_t header;
	uint8_t length;
	uint8_t payload[26];
} network_packet_t;

typedef struct {
	uint16_t node_address;
	nw_evt_handler_t nw_evt_handler;
	network_packet_t tx_packet;
	network_packet_t rx_packet;

} network_instance_t;

void network_init(uint16_t node_address, nw_evt_handler_t evt_handler);
void nw_update();
void nw_send(uint16_t dest_node, uint8_t *message, uint8_t length, nw_msg_type_t nw_msg_type);

#endif /*_NETWORK_H_ */
