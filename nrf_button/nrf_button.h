/*
 * button.h
 *  Created on: 16 Mar 2023
 *      Author: pdlsurya
 */

#ifndef BUTTON_H_
#define BUTTON_H_

typedef enum
{
  NRF_BUTTON_0,
  NRF_BUTTON_1,
  NRF_BUTTON_2,
  NRF_BUTTON_3,
  NRF_BUTTON_4,
  NRF_BUTTON_5,
  NRF_BUTTON_6,
  NRF_BUTTON_7
} button_id_t;

typedef void (*button_evt_handler_t)(void);

void nrf_button_register(button_id_t button_id, uint8_t pin_number, button_evt_handler_t evt_handler);

void nrf_button_init();

#endif /* BUTTON_H_ */
