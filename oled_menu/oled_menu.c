/**
 * @file oled_menu.c
 * @author Surya Poudel
 * @brief
 * @version 0.1
 * @date 2023-03-16
 *
 * @copyright Copyright (c) 2023, Surya Poudel
 *
 */
#include "stdint.h"
#include "string.h"
#include "stddef.h"
#include "qdec_driver.h"
#include "oled_SH1106.h"
#include "nrf_button.h"
#include "oled_menu.h"
#include "boards.h"
#include "softTimer.h"
#include "math.h"
#include "nrf_delay.h"
#include "pwm_led.h"
#include "debug_log.h"
#include "nrf52_ble.h"
#include "mySdFat.h"

#define ROT_ENC_BUTTON NRF_GPIO_PIN_MAP(1, 15)
#define QDEC_PIN_A NRF_GPIO_PIN_MAP(1, 13)
#define QDEC_PIN_B NRF_GPIO_PIN_MAP(1, 10)
#define ITEM_COUNT 6

const char *items[] =
    {"Brightness", "Language", "Led blinker", "Draw wave", "BLE Advertising",
     "PWM LED"};

const char *language[4] =
    {"English", "Nepali", "Hindi", "German"};

uint8_t main_menu_start;
uint8_t main_menu_end = 3;

volatile bool back = true;
volatile bool enter = false;
volatile bool dir_cw = false;
volatile bool index_counter_updated = false;
uint8_t index_counter;
uint8_t default_brightness_level = 31;
uint8_t default_duty = 5;
uint8_t saved_index_counter;
bool led_state = false;
bool led_blink_flag = false;

Menu_state_t menu_state = MAIN_MENU;

SOFT_TIMER_DEF(led_timer);

#define BAR_IND_SIZE round((float)(62.0 / ITEM_COUNT))

void vertical_bar(uint8_t index)
{
  oled_drawLine(124, 0, 124, 63, true);
  oled_drawLine(127, 0, 127, 63, true);
  oled_drawLine(124, 0, 127, 0, true);
  oled_drawLine(124, 63, 127, 63, true);

  oled_drawLine(125, BAR_IND_SIZE * index, 125,
                (index == ITEM_COUNT - 1) ? 62 : BAR_IND_SIZE * (index + 1),
                true);
  oled_drawLine(126, BAR_IND_SIZE * index, 126,
                (index == ITEM_COUNT - 1) ? 62 : BAR_IND_SIZE * (index + 1),
                true);
}

void main_menu()
{
  for (uint8_t i = main_menu_start; i <= main_menu_end; i++)
    oled_printString(items[i], 0, (i - main_menu_start) * 16, 16, false);
  vertical_bar(index_counter);
}

void highlight(const char *str, uint8_t ypos)
{
  oled_setCursor(0, ypos);
  for (uint8_t i = 0; i < 123; i++)
    oled_writeByte(0xff);

  oled_setCursor(0, ypos + 8);
  for (uint8_t i = 0; i < 123; i++)
    oled_writeByte(0xff);
  oled_printString(str, 0, ypos, 16, true);
  oled_display();
}

void return_to_main_menu()
{
  oled_clearDisplay();
  main_menu();
  highlight(items[saved_index_counter],
            (saved_index_counter - main_menu_start) * 16);
}

void set_brightness()
{

  if (index_counter_updated)
  {

    if (dir_cw)
    {
      if (index_counter > 62)
        index_counter = 62;
    }
    else
    {
      if (index_counter > 250)
        index_counter = 0;
    }

    default_brightness_level = index_counter;
    oled_clearDisplay();
    oled_printString("<Brightness>", 12, 0, 16, true);
    oled_setBar(index_counter * 2, 4);
    char temp_disp[14] =
        {0};
    sprintf(temp_disp, "Brightness=%d%%",
            (uint8_t)map(index_counter, 0, 62, 0, 100));
    oled_printString(temp_disp, 7, 40, 16, false);
    oled_display();
    nrf_delay_ms(26);
    oled_setContrast(index_counter * 4);
    index_counter_updated = false;
  }
}

void language_select()
{

  if (index_counter_updated)
  {

    if (dir_cw)
    {
      index_counter = index_counter % 4;
    }
    else
    {
      if (index_counter > 250)
        index_counter = 3;
    }
    oled_clearDisplay();
    for (uint8_t i = 0; i < 4; i++)
      oled_printString(language[i], 0, i * 16, 16, false);
    highlight(language[index_counter], index_counter * 16);
    index_counter_updated = false;
  }
}

void led_blink_evt_handler()
{
  if (!led_blink_flag)
    led_blink_flag = true;
}

void led_blink()
{
  if (index_counter_updated)
  {
    softTimer_stop(&led_timer);
    if (dir_cw)
    {
      if (index_counter > 20)
        index_counter = 20;
    }

    else
    {

      if (index_counter > 250 || index_counter == 0) // 8-bit unsigned int ranges from 0 to 255
        index_counter = 1;
    }

    for (uint8_t i = 3; i <= 7; i++)
      oled_clearPart(i, 0, 128);
    oled_printString("->Press to Back", 0, 40, 16, false);
    char temp_disp[15] =
        {0};
    sprintf(temp_disp, "Delay=%d ms", index_counter * 50);
    oled_printString(temp_disp, 0, 24, 16, false);
    oled_display();
    index_counter_updated = false;
    softTimer_start(&led_timer, MS_TO_TICKS(index_counter * 50));
  }

  if (led_blink_flag)
  {
    bsp_board_led_invert(0);
    led_state = !led_state;
    oled_printString("LED Blinking!", 0, 0, 16, led_state);
    oled_display();
    led_blink_flag = false;
  }
}

void draw_wave()
{
  if (index_counter_updated)
  {

    if (dir_cw)
    {
      if (index_counter > 7)
        index_counter = 7;
    }
    else
    {
      if (index_counter < 1 || index_counter > 250)
        index_counter = 1;
    }
    oled_clearDisplay();
    oled_drawSine(index_counter * 0.01, 10, true);
    oled_display();
    index_counter_updated = false;
  }
}

void pwmLed_control()
{

  if (index_counter_updated)
  {

    if (dir_cw)
    {
      if (index_counter > 20)
        index_counter = 20;
    }
    else
    {
      if (index_counter < 1 || index_counter > 250)
        index_counter = 0;
    }

    default_duty = index_counter;
    pwm_led_control(index_counter * 5);
    oled_clearDisplay();
    oled_printString("<PWM LED CONTROL>", 12, 0, 6, true);
    oled_setBar(map(index_counter, 0, 20, 0, 125), 4);
    char temp_disp[14] =
        {0};
    sprintf(temp_disp, "Duty cycle=%d%%",
            (uint8_t)map(index_counter, 0, 20, 0, 100));
    oled_printString(temp_disp, 7, 40, 16, false);
    oled_display();
    nrf_delay_ms(26);
    index_counter_updated = false;
  }
}

void qdec_event_handler(int16_t sample_acc, qdec_dir_t direction)
{
  if (!index_counter_updated)
  {
    if (direction == DIR_CLOCKWISE)
    {
      index_counter++;
      dir_cw = true;
    }
    else if (direction == DIR_ANTI_CLOCKWISE)
    {
      index_counter--;
      dir_cw = false;
    }
    index_counter_updated = true;
  }
}

void rot_enc_button_evt_handler()
{

  if (!enter)
    enter = true;

  if (!back)
  {

    index_counter = saved_index_counter;
    back = true;
    NRF_PWM1->TASKS_STOP = 1;
    oled_resetLog();
    oled_setLineAddress(0);
    return_to_main_menu();
    softTimer_stop(&led_timer);
    ble_stop_advertising();
    bsp_board_led_off(0);
    menu_state = MAIN_MENU;
    enter = false;
  }
}

void ble_advertisement()
{
  ble_start_advertising();
}

void handle_menu_enter()
{
  saved_index_counter = index_counter;
  switch (index_counter)
  {
  case 0:
  {
    oled_clearDisplay();
    oled_printString("<Brightness>", 12, 0, 16, true);
    char temp_disp[14] =
        {0};
    sprintf(temp_disp, "Brightness=%d%%",
            (uint8_t)map(default_brightness_level, 0, 62, 0, 100));
    oled_printString(temp_disp, 7, 40, 16, false);
    oled_setBar(default_brightness_level * 2, 4);
    oled_display();
    index_counter = default_brightness_level;
    back = false;
    menu_state = SET_BRIGHTNESS;
    break;
  }

  case 1:
  {

    index_counter = 0;
    oled_clearDisplay();
    for (uint8_t i = 0; i < 4; i++)
      oled_printString(language[i], 0, i * 16, 16, false);
    highlight(language[index_counter], index_counter * 16);
    back = false;
    menu_state = SET_LANGUAGE;
    break;
  }

  case 2:
  {
    oled_clearDisplay();
    oled_printString("->Press to Back", 0, 40, 16, false);
    back = false;
    index_counter = 1;
    char temp_disp[15] =
        {0};
    sprintf(temp_disp, "Delay=%d ms", index_counter * 50);
    oled_printString(temp_disp, 0, 24, 16, false);
    oled_display();
    softTimer_start(&led_timer, MS_TO_TICKS(index_counter * 50));
    menu_state = LED_BLINKER;
    break;
  }
  case 3:
  {
    oled_clearDisplay();
    back = false;
    menu_state = DRAW_WAVE;
    index_counter = 1;
    oled_drawSine(index_counter * 0.01, 10, true);
    oled_display();
    break;
  }

  case 4:
  {
    back = false;
    menu_state = BLE_ADV;
    oled_clearDisplay();
    oled_printString("BLE Adv active!", 0, 0, 6, false);
    oled_display();
    break;
  }

  case 5:
  {
    index_counter = default_duty;
    pwm_led_control(index_counter * 5);
    oled_clearDisplay();
    oled_printString("<PWM LED CONTROL>", 12, 0, 6, true);
    oled_setBar(map(index_counter, 0, 20, 0, 125), 4);
    char temp_disp[14] =
        {0};
    sprintf(temp_disp, "Duty cycle=%d%%",
            (uint8_t)map(index_counter, 0, 20, 0, 100));
    oled_printString(temp_disp, 7, 40, 16, false);
    oled_display();
    back = false;
    menu_state = PWM_LED;
    break;
  }
  default:
    break;
  }
}

void menu_process()
{
  switch (menu_state)
  {

  case MAIN_MENU:
  {
    if (index_counter_updated)
    {

      if (dir_cw)
      {
        index_counter = index_counter % ITEM_COUNT;
      }

      else
      {
        if (index_counter > 250)
          index_counter = ITEM_COUNT - 1;
      }
      if (index_counter > 3)
      {
        main_menu_start = index_counter - 3;
        main_menu_end = index_counter;
      }
      else
      {
        main_menu_start = 0;
        main_menu_end = 3;
      }
      oled_clearDisplay();
      main_menu();
      highlight(items[index_counter],
                (index_counter - main_menu_start) * 16);
      index_counter_updated = false;
    }

    if (enter)
    {
      handle_menu_enter();
    }
    break;
  }

  case SET_BRIGHTNESS:
  {
    set_brightness();
    break;
  }

  case SET_LANGUAGE:
  {
    language_select();
    break;
  }

  case LED_BLINKER:
  {
    led_blink();
    break;
  }

  case DRAW_WAVE:
  {
    draw_wave();
    break;
  }

  case BLE_ADV:
  {
    ble_advertisement();
    break;
  }

  case PWM_LED:
  {
    pwmLed_control();
    break;
  }
  default:
    break;
  }
}

void oled_menu_init()
{

  nrf_button_register(NRF_BUTTON_3, ROT_ENC_BUTTON,
                      rot_enc_button_evt_handler);

  softTimer_create(&led_timer, led_blink_evt_handler, SOFT_TIMER_MODE_REPEATED);

  pwm_led_init();

  oled_setLineAddress(0);
  oled_clearDisplay();
  main_menu();
  highlight(items[main_menu_start], 0 * 16);

  qdec_config_t qdec_config;
  qdec_config.enable_debounce_filter = 1;
  qdec_config.pin_a = QDEC_PIN_A;
  qdec_config.pin_b = QDEC_PIN_B;
  qdec_config.qdec_evt_handler = qdec_event_handler;
  qdec_config.sample_period = 3;
  qdec_init(&qdec_config);
}
