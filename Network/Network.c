/**
 * @file Network.c
 * @author Surya Poudel (poudel.surya2011@gmail.com)
 * @brief Network layer driver for nRF24 protocol
 * @version 0.1
 * @date 2022-03-02
 *
 * @copyright Copyright(c) 2022, Surya Poudel
 */

#include "stdint.h"
#include "string.h"
#include "stdbool.h"
#include "Network.h"
#include "nrf24.h"
#include "debug_log.h"
#include "boards.h"

uint16_t mask_check = 0xFFFF;
uint16_t node_mask;

const uint8_t address_pool[7] = { 0xc3, 0x3c, 0x33, 0xce, 0x3e, 0xe3, 0xec };

uint8_t node_physical_address[5] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };

bool nw_available = false;

network_instance_t instance;

void physical_address(uint16_t node, uint8_t *result) {
	uint8_t count = 0;
	while (node) {
		result[count] = address_pool[(node % 8)];
		count++;
		node = node / 8;
	}
}

void setup_address() {
	while (mask_check & instance.node_address)
		mask_check <<= 3;

	node_mask = ~mask_check;

	physical_address(instance.node_address, node_physical_address);

}

bool is_descendent(uint16_t node) {
	return ((node & node_mask) == instance.node_address);
}

uint16_t next_hop_node(uint16_t node) {
	uint16_t direct_child_mask = ~((~node_mask) << 3);
	return node & direct_child_mask;
}

uint16_t parent_node() {
	return ((node_mask >> 3) & instance.node_address);
}

void nw_send(uint16_t dest_node, uint8_t *message, uint8_t length,nw_msg_type_t nw_msg_type) {

	uint8_t next_hop_physical_address[5] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
	uint16_t next_hop;

	memset(&instance.tx_packet, 0, sizeof(instance.tx_packet));

	//Set packet header and payload
	instance.tx_packet.header.from_node = instance.node_address;
	instance.tx_packet.header.to_node = dest_node;
	instance.tx_packet.header.msg_type = nw_msg_type;
	instance.tx_packet.length = length;
	memcpy(instance.tx_packet.payload, message, length);

	if (is_descendent(dest_node)) {
		next_hop = next_hop_node(dest_node);
	} else {
		next_hop = parent_node();
	}
	debug_log_print("next hop= 0%o", next_hop);
	physical_address(next_hop, next_hop_physical_address);

	debug_log_print("TX address= %X:%X:%X:%X:%X", next_hop_physical_address[0],
			next_hop_physical_address[1], next_hop_physical_address[2],
			next_hop_physical_address[3], next_hop_physical_address[4]);

	nrf24_set_tx_address(next_hop_physical_address);
	nrf24_set_mode(NRF24_MODE_TX);
	nrf24_send((uint8_t*) &instance.tx_packet, sizeof(instance.tx_packet));

}

void nw_update() {
	//if message is for this node
	if (instance.rx_packet.header.to_node == instance.node_address) {

		if (instance.rx_packet.header.msg_type == NW_MSG_DATA) {
			debug_log_print("Incoming message from node:-> 0%o",
					instance.rx_packet.header.from_node);
			instance.nw_evt_handler(instance.rx_packet.payload,
					instance.rx_packet.length);
			char ack_msg[26] = "";
			nw_send(instance.rx_packet.header.from_node, (uint8_t*) ack_msg,0, NW_MSG_ACK); //Send ACK message;
		}
		else if(instance.rx_packet.header.msg_type == NW_MSG_ACK){
			debug_log_print("ACK received from node:->0%o",instance.rx_packet.header.from_node);
		}

	}

	else {

		debug_log_print("Forwarding Message arrived!");
		uint16_t next_hop;
		uint8_t next_hop_physical_address[5] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };

		memset(&instance.tx_packet, 0, sizeof(instance.tx_packet));
		instance.tx_packet = instance.rx_packet;

		if (is_descendent(instance.tx_packet.header.to_node)) {
			debug_log_print("Descendent");
			next_hop = next_hop_node(instance.tx_packet.header.to_node);
		} else {
			debug_log_print("Not a Descendent");
			next_hop = parent_node();
		}

		physical_address(next_hop, next_hop_physical_address);
		debug_log_print("Next hop= 0%o", next_hop);
		debug_log_print("TX address= %X:%X:%X:%X:%X",
				next_hop_physical_address[0], next_hop_physical_address[1],
				next_hop_physical_address[2], next_hop_physical_address[3],
				next_hop_physical_address[4]);
		nrf24_set_tx_address(next_hop_physical_address);

		nrf24_set_mode(NRF24_MODE_TX);

		nrf24_send((uint8_t*) &instance.tx_packet,
				sizeof(instance.tx_packet) - 4);

	}
}

void nrf24_nw_evt_handler(nrf24_event_t *evt) {
	switch (evt->event_type) {
	case NRF24_DATA_READY:
		if (evt->data != NULL) {
			memset(&instance.rx_packet, 0, sizeof(instance.rx_packet));
			instance.rx_packet = *((network_packet_t*) evt->data);
			nw_update();
		}

		break;
	case NRF24_TX_SUCCESS:
		debug_log_print("TX SUCCESS!");
		//Tx completed, now switch to rx mode;
		nrf24_set_mode(NRF24_MODE_RX);

		break;
	case NRF24_TX_FAILED:
		debug_log_print("TX FAILED!");
		nrf24_set_mode(NRF24_MODE_RX);
		break;
	case NRF24_INVALID_OPERATION:
		debug_log_print("INVALID OPERATION!");
		break;
	default:
		break;
	}

}

void network_init(uint16_t node_address, nw_evt_handler_t evt_handler) {

	instance.nw_evt_handler = evt_handler;
	instance.node_address = node_address;
	setup_address();
	nrf24_init();
	nrf24_set_evt_handler(nrf24_nw_evt_handler);
	nrf24_set_rx_address(node_physical_address, 1);
	debug_log_print("RX address= %X:%X:%X:%X:%X", node_physical_address[0],
			node_physical_address[1], node_physical_address[2],
			node_physical_address[3], node_physical_address[4]);
	nrf24_set_mode(NRF24_MODE_RX);

}
