/*
 * qdec_driver.h
 *
 *  Created on: 16 Mar 2023
 *      Author: pdlsurya
 */
#include "stdbool.h"

#ifndef QDEC_DRIVER_H_
#define QDEC_DRIVER_H_

typedef enum{
  DIR_CLOCKWISE, DIR_ANTI_CLOCKWISE
}qdec_dir_t;

typedef void (*qdec_evt_handler_t)(int16_t, qdec_dir_t);

typedef struct{
  uint8_t pin_a;
  uint8_t pin_b;
  uint8_t sample_period;
  bool enable_debounce_filter;
  qdec_evt_handler_t qdec_evt_handler;
}qdec_config_t;


void qdec_init(qdec_config_t* config);

#endif /* QDEC_DRIVER_H_ */
