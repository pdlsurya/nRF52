/*
 * pwm_led.c
 *
 *  Created on: 23 Apr 2023
 *      Author: pdlsurya
 */
#include "pwm_led.h"
#include "nrf.h"
#include "boards.h"
#include "mySdFat.h"
#include "oled_SH1106.h"


#define PWM_LED_1 NRF_GPIO_PIN_MAP(0,8)
#define PWM_LED_2 NRF_GPIO_PIN_MAP(1,9)
#define PWM_LED_3 NRF_GPIO_PIN_MAP(0,12)

uint16_t pwm_seq0;


void pwm_led_control(uint8_t duty_cycle){

  pwm_seq0=map(duty_cycle,0,100,0,32767);
  NRF_PWM1->TASKS_SEQSTART[0]=1;
}


void pwm_led_init(){

   nrf_gpio_cfg_output (PWM_LED_1);
   nrf_gpio_cfg_output (PWM_LED_2);
   nrf_gpio_cfg_output (PWM_LED_3);

    NRF_PWM1->PSEL.OUT[0] = PWM_LED_1
        | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
    NRF_PWM1->PSEL.OUT[1] = PWM_LED_2
           | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
    NRF_PWM1->PSEL.OUT[2] = PWM_LED_3
           | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);


    NRF_PWM1->PSEL.OUT[3] = 0xFFFFFFFF;

    NRF_PWM1->ENABLE = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);
    NRF_PWM1->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);
    NRF_PWM1->PRESCALER = (PWM_PRESCALER_PRESCALER_DIV_8 <<
    PWM_PRESCALER_PRESCALER_Pos);
    NRF_PWM1->COUNTERTOP = ( 32767<< PWM_COUNTERTOP_COUNTERTOP_Pos);
    NRF_PWM1->LOOP = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);
    NRF_PWM1->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos)
        | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    NRF_PWM1->SEQ[0].PTR = ((uint32_t) (&pwm_seq0) << PWM_SEQ_PTR_PTR_Pos);
    NRF_PWM1->SEQ[0].CNT = ((sizeof(pwm_seq0) / sizeof(uint16_t)) <<
    PWM_SEQ_CNT_CNT_Pos);
    NRF_PWM1->SEQ[0].REFRESH = 0;
    NRF_PWM1->SEQ[0].ENDDELAY = 0;
}
