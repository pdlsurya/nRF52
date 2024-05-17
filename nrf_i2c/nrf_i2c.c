/*
 * nrf_i2c.c
 *
 *  Created on: 14 Mar 2023
 *      Author: pdlsurya
 */

#include "nrf_i2c.h"
#include "boards.h"
#include "nrf.h"
#include "string.h"
#include "nrf_delay.h"
#include "stdbool.h"
#include "nrf_gpio.h"

bool tx_started = false;

#define SCL_PIN_INIT_CONF \
  ((GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos) | (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos))

#define SDA_PIN_INIT_CONF SCL_PIN_INIT_CONF

i2c_instance_t *p_instance_core;

void i2c_init(i2c_instance_t *p_instance, i2c_config_t *p_config,
              i2c_evt_handler_t i2c_evt_handler)
{

  p_instance->i2c_evt_hanlder = i2c_evt_handler;
  p_instance_core = p_instance;
#if NRF_TWI_ENABLED
  // Disable the TWIM module while we reconfigure it
  NRF_TWI0->ENABLE = TWI_ENABLE_ENABLE_Disabled << TWI_ENABLE_ENABLE_Pos;
  NRF_TWIM0->SHORTS = 0;
  NVIC_DisableIRQ(SPI0_TWI0_IRQn);
  NVIC_ClearPendingIRQ(SPI0_TWI0_IRQn);

  NRF_TWI0->PSEL.SCL = 24UL;
  NRF_TWI0->PSEL.SDA = 22UL;

  NRF_P0->PIN_CNF[24] = SCL_PIN_INIT_CONF;
  NRF_P0->PIN_CNF[22] = SDA_PIN_INIT_CONF;
  NRF_TWI0->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K400;

  NRF_TWI0->ENABLE = TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos;
#elif NRF_TWIM_ENABLED
  // Disable the TWIM module while we reconfigure it

  p_instance->i2c_reg->ENABLE = TWIM_ENABLE_ENABLE_Disabled
                                << TWIM_ENABLE_ENABLE_Pos;
  p_instance->i2c_reg->SHORTS = 0;
  NVIC_DisableIRQ(SPI0_TWI0_IRQn);
  NVIC_ClearPendingIRQ(SPI0_TWI0_IRQn);

  p_instance->i2c_reg->FREQUENCY = p_config->frequency;
  p_instance->i2c_reg->PSEL.SCL = p_config->scl_pin;
  p_instance->i2c_reg->PSEL.SDA = p_config->sda_pin;

  NRF_P0->PIN_CNF[p_config->scl_pin] = SCL_PIN_INIT_CONF;
  NRF_P0->PIN_CNF[p_config->sda_pin] = SDA_PIN_INIT_CONF;

  p_instance->i2c_reg->SHORTS = TWIM_SHORTS_LASTTX_STOP_Enabled
                                << TWIM_SHORTS_LASTTX_STOP_Pos;
  // Disable the EasyDMA list functionality for TWI TX
  p_instance->i2c_reg->TXD.LIST = TWIM_TXD_LIST_LIST_Disabled
                                  << TWIM_TXD_LIST_LIST_Pos;

  p_instance->i2c_reg->INTENSET |= TWIM_INTENSET_LASTTX_Enabled
                                   << TWIM_INTENSET_LASTTX_Pos;

  NVIC_SetPriority(SPI0_TWI0_IRQn, 3);
  NVIC_EnableIRQ(SPI0_TWI0_IRQn);

  p_instance->i2c_reg->ENABLE = TWIM_ENABLE_ENABLE_Enabled
                                << TWIM_ENABLE_ENABLE_Pos;

#endif
}

bool i2c_tx(i2c_instance_t *p_instance, uint8_t i2c_address, uint8_t *tx_data,
            uint8_t tx_len)
{

#if NRF_TWI_ENABLED
  for (uint8_t i = 0; i < tx_len; i++)
  {

    NRF_TWI0->ADDRESS = i2c_address;
    if (!tx_started)
    {
      NRF_TWI0->TASKS_STARTTX = 1UL;
      tx_started = true;
    }
    NRF_TWI0->TXD = tx_data[i];
    while (NRF_TWI0->EVENTS_TXDSENT == 0)
    {
      if (NRF_TWI0->EVENTS_ERROR == 1)
        return 0;
    }
    NRF_TWI0->EVENTS_TXDSENT = 0UL;
  }
  NRF_TWI0->TASKS_STOP = 1;
  while (NRF_TWI0->EVENTS_STOPPED == 0UL)
    ;
  NRF_TWI0->EVENTS_STOPPED = 0UL;
  tx_started = false;
  return 1;
#elif NRF_TWIM_ENABLED

  p_instance->i2c_reg->ADDRESS = i2c_address;
  p_instance->i2c_reg->TXD.PTR = (uint32_t)tx_data;
  p_instance->i2c_reg->TXD.MAXCNT = tx_len;
  p_instance->i2c_reg->TASKS_STARTTX = 1UL;

  if (tx_len <= 2)
  {

    NVIC_DisableIRQ(SPI0_TWI0_IRQn);
    NVIC_ClearPendingIRQ(SPI0_TWI0_IRQn);

    while (p_instance->i2c_reg->EVENTS_LASTTX == 0)
      ;

    p_instance->i2c_reg->EVENTS_LASTTX = 0;

    NVIC_SetPriority(SPI0_TWI0_IRQn, 3);
    NVIC_EnableIRQ(SPI0_TWI0_IRQn);
  }

  return 1;
#endif
}

// isr handler
void SPI0_TWI0_IRQHandler()
{

  p_instance_core->i2c_evt_hanlder();
}
