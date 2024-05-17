/*
 * nrf_i2c.h
 *
 *  Created on: 14 Mar 2023
 *      Author: pdlsurya
 */

#ifndef NRF_I2C_H_
#define NRF_I2C_H_

#include "stdint.h"
#include "stdbool.h"
#include "boards.h"

#define NRF_TWIM_ENABLED 0
#define NRF_TWI_ENABLED 1

#define I2C_INSTANCE_DEF(instance, id) \
  i2c_instance_t instance= {.i2c_reg= NRF_TWIM##id }

#define GET_IT_DONE(iddd) \
  do { \
    if(1){\
       debug_log_print("%d",(iddd));\
    }\
  } while(0)


typedef void (*i2c_evt_handler_t) (void);

typedef struct
{
  uint8_t scl_pin;
  uint8_t sda_pin;
  uint32_t frequency;

} i2c_config_t;

typedef struct
{
  NRF_TWIM_Type *i2c_reg;
  i2c_evt_handler_t i2c_evt_hanlder;

} i2c_instance_t;

void
i2c_init (i2c_instance_t *p_instance, i2c_config_t *config,
	  i2c_evt_handler_t evt_handler);

bool
i2c_tx (i2c_instance_t *p_instance, uint8_t i2c_address, uint8_t *tx_data,
	uint8_t tx_len);

#endif /* NRF_I2C_H_ */
