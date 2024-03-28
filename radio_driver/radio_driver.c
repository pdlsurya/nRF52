/**
 * @file radio_driver.c
 * @author Surya Poudel
 * @brief Radio peripheral driver for nrf52840
 * @version 1.0
 * @date 2022-04-26
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdint.h>
#include <string.h>
#include "boards.h"
#include "radio_driver.h"

#define RADIO_IRQ_PRIORITY 6

radio_event_handler_t radio_event_handler;

void radio_set_evt_handler(radio_event_handler_t evt_handler)
{
    radio_event_handler = evt_handler;
}

void radio_enable_mode(radio_mode_t mode)
{
    switch (mode)
    {
    case MODE_TX:

        NRF_RADIO->TASKS_TXEN = 1;
        break;
    case MODE_RX:
        NRF_RADIO->TASKS_RXEN = 1;
        break;
    default:
        break;
    }
}

radio_state_t radio_get_state()
{
    return NRF_RADIO->STATE;
}

inline void radio_disable()
{

    NRF_RADIO->TASKS_DISABLE = 1U;
    while (NRF_RADIO->EVENTS_DISABLED == 0)
        ;

    NRF_RADIO->EVENTS_DISABLED = 0;
}
void radio_set_mode(radio_mode_t mode)
{
    NRF_RADIO->TASKS_DISABLE = 1U;
    while (NRF_RADIO->EVENTS_DISABLED == 0)
        ;

    NRF_RADIO->EVENTS_DISABLED = 0;

    if (radio_get_state() == DISABLED)
    {
        radio_enable_mode(mode);

        while (NRF_RADIO->EVENTS_READY == 0)
            ;

        NRF_RADIO->EVENTS_READY = 0;
    }
}

void radio_tx()
{
    if (radio_get_state() == DISABLED)
    {
        NRF_RADIO->TASKS_TXEN = 1U;

        while (NRF_RADIO->EVENTS_READY == 0U)
            ;

        NRF_RADIO->EVENTS_READY = 0U;
    }

    NRF_RADIO->TASKS_START = 1U;

    while (NRF_RADIO->EVENTS_END == 0U)
        ;

    NRF_RADIO->EVENTS_END = 0U;

    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0U)
        ;

    NRF_RADIO->EVENTS_DISABLED = 0U;
}

uint8_t radio_get_crc_status()
{
    return NRF_RADIO->CRCSTATUS;
}

inline void radio_start_rx()
{
    if (radio_get_state() == RX_IDLE)
    {

        NRF_RADIO->TASKS_START = 1U;
    }
}

void radio_start_tx()
{
    if (radio_get_state() == TX_IDLE)
    {

        NRF_RADIO->TASKS_START = 1U;
        while (NRF_RADIO->EVENTS_END == 0U)
            ;
        NRF_RADIO->EVENTS_END = 0U;
    }
}

void radio_set_frequency(uint32_t frequency)
{
    NRF_RADIO->FREQUENCY = frequency;
}

void radio_set_tx_power(uint32_t tx_power)
{
    NRF_RADIO->TXPOWER = tx_power;
}

void radio_set_data_rate(radio_data_rate_t data_rate)
{
    NRF_RADIO->MODE = (uint32_t)data_rate;
}

void radio_config_pl_size(uint8_t max_pln, bool dynamic_pl)
{
    if (dynamic_pl)
    {
        NRF_RADIO->PCNF0 |= 8 << 0;  // No. of bits of LEN field
        NRF_RADIO->PCNF0 |= 1 << 8;  // No. of bytes of S0 field
        NRF_RADIO->PCNF0 |= 0 << 16; // No. of bits of S1 field
        NRF_RADIO->PCNF0 |= 0 << 26; // Include or exclude CRC in total length
    }
    else
    {
        uint32_t mask = 0xFFFF0000;
        NRF_RADIO->PCNF1 &= mask;
        NRF_RADIO->PCNF1 |= (uint32_t)max_pln << 8;
    }

    NRF_RADIO->PCNF1 |= (uint32_t)max_pln << 0;
}

void radio_set_address_width(uint8_t add_width)
{
    NRF_RADIO->PCNF1 |= ((uint32_t)(add_width - 1)) << 16;
}

void radio_set_payload_endian(radio_endian_t endian)
{
    if (endian == RADIO_BIG_ENDIAN)
        NRF_RADIO->PCNF1 |= ((uint32_t)1) << 24;
    else
        NRF_RADIO->PCNF1 |= ((uint32_t)0) << 24;
}

void radio_enable_whitening(bool en)
{
    if (en)
        NRF_RADIO->PCNF1 |= ((uint32_t)1U) << 25;
}

uint8_t radio_get_received_address()
{
    return NRF_RADIO->RXMATCH;
}

static void radio_set_tx_logical_address(uint8_t logical_address)
{
    NRF_RADIO->TXADDRESS = (uint32_t)logical_address;
}

static void radio_set_rx_logical_address(uint8_t logical_address)
{
    NRF_RADIO->RXADDRESSES |= 1UL << logical_address;
}

void radio_set_address(const uint8_t *address, uint8_t logical_address)
{

    uint32_t prefix_mask = 0x000000FF << (logical_address * 8);
    if (logical_address < 4)
    {
        NRF_RADIO->PREFIX0 &= ~prefix_mask;
        NRF_RADIO->PREFIX0 |= (uint32_t)(address[0] << (logical_address * 8));
    }
    else
    {
        NRF_RADIO->PREFIX1 &= ~prefix_mask;
        NRF_RADIO->PREFIX1 |= (uint32_t)(address[0] << (logical_address * 8));
    }

    if (logical_address == 0)
    {
        NRF_RADIO->BASE0 = 0UL;
        for (uint8_t i = 0; i < 4; i++)
            NRF_RADIO->BASE0 |= (uint32_t)(address[4 - i] << (8 * i));
    }
    else if (logical_address > 0 && logical_address <= 7)
    {
        NRF_RADIO->BASE0 = 0UL;
        for (uint8_t i = 0; i < 4; i++)
            NRF_RADIO->BASE1 |= (uint32_t)(address[4 - i] << (8 * i));
    }
    radio_set_tx_logical_address(logical_address);
}

void radio_set_tx_address(const uint8_t *tx_address, uint8_t logical_address)
{
    radio_set_address(tx_address, logical_address);

    radio_set_tx_logical_address(logical_address);
}

void radio_set_rx_address(const uint8_t *rx_address, uint8_t logical_address)
{

    radio_set_address(rx_address, logical_address);

    radio_set_rx_logical_address(logical_address);
}

void radio_configure_crc(uint8_t crc_len, uint8_t crc_add, uint32_t crc_poly, uint32_t crc_init_val)
{
    NRF_RADIO->CRCCNF |= ((uint32_t)crc_len) << 0;

    NRF_RADIO->CRCCNF |= ((uint32_t)crc_add) << 8;

    NRF_RADIO->CRCPOLY = crc_poly;
    NRF_RADIO->CRCINIT = crc_init_val;
}

void radio_set_whiteiv(uint8_t init_value)
{
    NRF_RADIO->DATAWHITEIV = (uint32_t)(init_value);
}

void radio_set_packet_ptr(uint8_t *ptr)
{
    NRF_RADIO->PACKETPTR = (uint32_t)ptr;
}

void radio_enable_interrupts()
{
    NRF_RADIO->INTENSET = 1UL << 3;

    NVIC_SetPriority(RADIO_IRQn, RADIO_IRQ_PRIORITY);
    NVIC_EnableIRQ(RADIO_IRQn);
}

void RADIO_IRQHandler(void)
{

    if ((NRF_RADIO->EVENTS_END) && (radio_get_state() == RX_IDLE))
    {
        NRF_RADIO->EVENTS_END = 0;

        radio_event_handler();
    }
}
