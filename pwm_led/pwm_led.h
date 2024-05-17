/*
 * pwm_led.h
 *
 *  Created on: 23 Apr 2023
 *      Author: pdlsurya
 */

#ifndef CUSTOM_LIB_PWM_LED_PWM_LED_H_
#define CUSTOM_LIB_PWM_LED_PWM_LED_H_

#include "stdint.h"

void pwm_led_init();

void pwm_led_control(uint8_t duty_cycle);


#endif /* CUSTOM_LIB_PWM_LED_PWM_LED_H_ */
