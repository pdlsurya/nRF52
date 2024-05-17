#include "boards.h"
#include "stdint.h"
#include "qdec_driver.h"
#include "debug_log.h"


qdec_evt_handler_t qdec_evt_handler;

volatile int16_t curr_acc;
volatile int16_t prev_acc;
volatile qdec_dir_t direction;


void qdec_init(qdec_config_t* config){
    qdec_evt_handler=config->qdec_evt_handler;

    nrf_gpio_cfg_input (config->pin_a, 0);
    nrf_gpio_cfg_input (config->pin_b, 0);

    NRF_QDEC->PSEL.A=config->pin_a;
    NRF_QDEC->PSEL.B=config->pin_b;
    NRF_QDEC->PSEL.LED|=1UL <<31;

    NRF_QDEC->DBFEN=config->enable_debounce_filter;

    NRF_QDEC->SAMPLEPER=config->sample_period;
    NRF_QDEC->REPORTPER=1;

    NRF_QDEC->INTENSET=1;
    NRF_QDEC->INTENSET |=1UL <<1;

    NVIC_EnableIRQ (QDEC_IRQn);
    NVIC_SetPriority (QDEC_IRQn,5);

    NRF_QDEC->ENABLE=1UL;

    NRF_QDEC->TASKS_START=1UL;

}


void QDEC_IRQHandler(void){
  if(NRF_QDEC->EVENTS_REPORTRDY)
    {
     NRF_QDEC->EVENTS_REPORTRDY=0;

     curr_acc=NRF_QDEC->ACC / 4;

  if(curr_acc!=prev_acc)
    {
   qdec_evt_handler(curr_acc,direction);
   prev_acc=curr_acc;

    }
}
  if(NRF_QDEC->EVENTS_SAMPLERDY)
    {
      NRF_QDEC->EVENTS_SAMPLERDY=0;
      if(NRF_QDEC->SAMPLE == -1)
	direction=DIR_ANTI_CLOCKWISE;
      else if (NRF_QDEC->SAMPLE == 1)
	direction=DIR_CLOCKWISE;
     }
}
