/*
 * oled_menu.h
 *
 *  Created on: 16 Mar 2023
 *      Author: pdlsurya
 */

#ifndef OLED_MENU_H_
#define OLED_MENU_H_

#include "stdint.h"
#include "stdbool.h"

typedef enum {
  MAIN_MENU, SET_BRIGHTNESS, SET_LANGUAGE, LED_BLINKER, DRAW_WAVE,BLE_ADV,MUSIC_PLAYER, PWM_LED
} Menu_state_t;

void oled_menu_init();
void menu_process();

extern Menu_state_t menu_state;



#endif /*OLED_MENU_H_ */
