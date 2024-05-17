/*
 * button.c
 *
 *  Created on: 16 Mar 2023
 *      Author: pdlsurya
 */

#include "boards.h"
#include "nrf_button.h"
#include "softTimer.h"

#define BUTTON_DEBOUNCE_DELAY MS_TO_TICKS(100)

SOFT_TIMER_DEF(button_debounce_timer);

button_evt_handler_t evt_handler_queue[8] = {NULL};

void button_detection_evt_handler()
{
  for (uint8_t i = NRF_BUTTON_0; i <= NRF_BUTTON_7; i++)
  {
    if (NRF_GPIOTE->EVENTS_IN[i])
    {
      NRF_GPIOTE->EVENTS_IN[i] = 0;
      evt_handler_queue[i]();
    }
  }
}

void nrf_button_init()
{
  softTimer_create(&button_debounce_timer, button_detection_evt_handler, SOFT_TIMER_MODE_SINGLE_SHOT);
  NVIC_EnableIRQ(GPIOTE_IRQn);
  NVIC_SetPriority(GPIOTE_IRQn, GPIOTE_CONFIG_IRQ_PRIORITY);
}

void nrf_button_register(button_id_t button_id, uint8_t button_pin, button_evt_handler_t evt_handler)
{
  evt_handler_queue[button_id] = evt_handler;

  nrf_gpio_cfg_input(button_pin, 3);
  NRF_GPIOTE->CONFIG[button_id] = 1;
  NRF_GPIOTE->CONFIG[button_id] |= (uint32_t)(button_pin << 8);
  NRF_GPIOTE->CONFIG[button_id] |= (uint32_t)(2 << 16);
  NRF_GPIOTE->INTENSET |= 1UL << button_id;
}

void GPIOTE_IRQHandler(void)
{
  softTimer_start(&button_debounce_timer, BUTTON_DEBOUNCE_DELAY);
}
