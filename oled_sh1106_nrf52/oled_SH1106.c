/**
 * @file oled_SH1106.c
 * @author Surya Pudel
 * @brief  SH1106 oled library
 * @version 0.1
 * @date 2021-06-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "math.h"
#include "app_error.h"
#include "string.h"
#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include "boards.h"
#include "nrf_i2c.h"
#include "nrf_delay.h"
#include "font.h"
#include "oled_SH1106.h"

#define OLED_ADDRESS 0x3C
#define DISP_ON 0xAF
#define DISP_OFF 0xAE
#define NORM_MODE 0xA6
#define REV_MODE 0xA7
#define PAGE_ADDRESSING_MODE 0x02
#define CONTRAST_CTRL_MODE 0X81
#define TYPE_CMD 0x00
#define TYPE_DATA 0x40
#define CMD_ROL 0xA0
#define CMD_SCAN_COM63 0xC0
#define CMD_START_RMW 0xE0 // Read-Modify-Write start
#define CMD_STOP_RMW 0xEE  // Read-Modify-Write end

#define min(x,y) ((x)<(y)? (x):(y))

#define max(x,y ) ((x)>(y)? (x):(y))

#define PI 3.1416

#define sq(x) ((x)*(x))

static uint8_t log_row;
static bool log_scroll;

uint8_t char_cnt;
uint8_t num_cnt;
uint8_t battery[14] =
  { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
      0xC3, 0x3C };
uint8_t signal[9] =
  { 0x80, 0x00, 0xC0, 0x00, 0xF0, 0x00, 0xFC, 0x00, 0xFF };
uint8_t bar[128];

uint8_t dispBuffer[8][128];
uint8_t disp_row;
uint8_t disp_column;

static I2C_INSTANCE_DEF(i2c_instance,0);

uint8_t oled_page = 0;
bool oled_page_updated = false;
bool oled_display_in_progress = false;
uint8_t dispBuffer_temp[8][128];
uint8_t i2c_tx_buf_temp[129];

void
oled_sendCommand (uint8_t cmd)
{
  uint8_t cmd_buf[2] =
    { 0 };
  cmd_buf[0] = TYPE_CMD;
  cmd_buf[1] = cmd;
  i2c_tx (&i2c_instance, OLED_ADDRESS, cmd_buf, 2);
}

void
oled_setCursorPos (uint8_t x, uint8_t y)
{

  // the SH1106 display starts at x = 2! (there are two columns of off screen pixels)
  x += 2;
  oled_sendCommand (0xB0 + (y >> 3));
  // set lower column address  (00H - 0FH) => need the upper half only - THIS IS THE x, 0->127
  oled_sendCommand ((x & 0x0F));
  // set higher column address (10H - 1FH) => 0x10 | (2 >> 4) = 10
  oled_sendCommand (0x10 + (x >> 4));
}

void
oled_setCursor (uint8_t x, uint8_t y)
{

  disp_column = x;
  disp_row = y / 8;
}

void i2c_evt_handler ()
     {
  if (i2c_instance.i2c_reg->EVENTS_LASTTX == 1)
    {
      i2c_instance.i2c_reg->EVENTS_LASTTX = 0;
      oled_page++;

      if (oled_page == 8)
	{
	  oled_display_in_progress = false;
	  oled_page = 0;
	  return;
	}
      i2c_tx_buf_temp[0] = TYPE_DATA;
      memcpy (&i2c_tx_buf_temp[1], dispBuffer_temp[oled_page], 128);
      oled_setCursorPos (0, oled_page * 8);
      i2c_tx (&i2c_instance, OLED_ADDRESS, i2c_tx_buf_temp, 129);
    }

}

void oled_display ()
{

  if (!oled_display_in_progress)
    {
      oled_display_in_progress = true;

      memcpy (dispBuffer_temp, dispBuffer, sizeof(dispBuffer));

      i2c_tx_buf_temp[0] = TYPE_DATA;
      memcpy (&i2c_tx_buf_temp[1], dispBuffer_temp[oled_page], 128);

      oled_setCursorPos (0, oled_page * 8);
      i2c_tx (&i2c_instance, OLED_ADDRESS, i2c_tx_buf_temp, 129);

    }

}

void oled_setPageMode ()
{
  oled_sendCommand (0x20);
  oled_sendCommand (PAGE_ADDRESSING_MODE);
}

void oled_setChargePump ()
{
  oled_sendCommand (0x8d);
  oled_sendCommand (0x14);
}

void oled_setDisplayStart (uint8_t Start)
{
  oled_sendCommand (0xD3);
  oled_sendCommand (Start);
}

void oled_setLineAddress (uint8_t Address)
{

  oled_sendCommand (0x40 | Address);
}

void oled_clearPart (uint8_t page, uint8_t start_pos, uint8_t end_pos)
{

  oled_setCursor (start_pos, page * 8);
  for (uint8_t i = start_pos; i <= end_pos; i++)
    dispBuffer[disp_row][disp_column++] = 0x00;

}

void oled_update_page (uint8_t page, uint8_t page_buff[])
{
  oled_setCursorPos (0, page * 8);

  oled_setCursorPos (0, page * 8);
  uint8_t temp_buff[129] =
    { 0 };
  temp_buff[0] = TYPE_DATA;
  for (uint8_t col = 0; col < 128; col++)
    temp_buff[col + 1] = dispBuffer[page][col];

  i2c_tx (&i2c_instance, OLED_ADDRESS, temp_buff, 129);
}

void oled_clearDisplay ()
{

  for (uint8_t page = 0; page < 8; page++)
    {
      for (uint8_t column = 0; column < 128; column++)
	dispBuffer[page][column] = 0;
    }

}

void
oled_init ()
{
  i2c_config_t config;
  config.frequency = TWIM_FREQUENCY_FREQUENCY_K400;
  config.scl_pin = NRF_GPIO_PIN_MAP(0, 24);
  config.sda_pin = NRF_GPIO_PIN_MAP(0, 22);
  i2c_init (&i2c_instance, &config, i2c_evt_handler);
  oled_sendCommand (DISP_ON);
  oled_sendCommand (NORM_MODE);
  oled_sendCommand (CMD_ROL);        // Rotate 90 degrees
  oled_sendCommand (CMD_SCAN_COM63); // start scan from COM63 to COM0
  oled_setPageMode ();
  oled_setChargePump ();
  oled_clearDisplay ();
  oled_display ();
  nrf_delay_ms(27);
}

void
oled_printChar (char C, uint8_t xpos, uint8_t ypos, uint8_t font_size,
bool Highlight)
{
  char chr;

  if (font_size == 6)
    {
      for (uint8_t i = 0; i < 6; i++)
	{
	  chr = font6x8[((int) C - 32) * 6 + i];
	  if (Highlight)
	    chr = ~chr;

	  dispBuffer[disp_row][disp_column++] = chr;
	}
    }
  else if (font_size == 16)
    {
      oled_setCursor (xpos + (char_cnt) * 8, ypos);

      for (uint8_t i = 0; i < 8; i++)
	{
	  chr = font16x8[((int) C - 32) * 16 + i];
	  if (Highlight)
	    chr = ~chr;
	  dispBuffer[disp_row][disp_column++] = chr;
	}

      oled_setCursor (xpos + (char_cnt) * 8, ypos + 8);

      for (uint8_t i = 8; i < 16; i++)
	{
	  chr = font16x8[((int) C - 32) * 16 + i];
	  if (Highlight)
	    chr = ~chr;

	  dispBuffer[disp_row][disp_column++] = chr;
	}

      char_cnt++;
    }
  else
    return;
}

void
oled_printString (const char *str, uint8_t xpos, uint8_t ypos,
		  uint8_t font_size, bool Highlight)
{
  uint8_t len = strlen (str);
  char_cnt = 0;
  oled_setCursor (xpos, ypos);
  for (uint8_t i = 0; i < len; i++)
    oled_printChar (str[i], xpos, ypos, font_size, Highlight);
  char_cnt = 0;
}

void
oled_print7Seg_digit (char C, uint8_t xpos, uint8_t ypos)
{
  char chr;

  for (uint8_t column = 0; column < 16; column++)
    {
      for (uint8_t page = 0; page < 3; page++)
	{
	  oled_setCursor (xpos + column + (num_cnt * 16), ypos + page * 8);
	  chr = font_7SEG[((int) C - 48) * 48 + (page + column * 3)];
	  dispBuffer[disp_row][disp_column++] = chr;
	}
    }

  num_cnt++;
}

void
oled_print7Seg_number (const char *str, uint8_t xpos, uint8_t ypos)
{
  uint8_t len = strlen (str);
  num_cnt = 0;
  oled_setCursor (xpos, ypos);

  for (uint8_t i = 0; i < len; i++)
    oled_print7Seg_digit (str[i], xpos, ypos);
  num_cnt = 0;
}

void
oled_setContrast (uint8_t level)
{

  oled_sendCommand (CONTRAST_CTRL_MODE);
  oled_sendCommand (level);
}

void
oled_writeByte (uint8_t byte)
{

  dispBuffer[disp_row][disp_column++] = byte;

}

void
oled_setPixel (uint8_t x, uint8_t y, bool set)
{
  uint8_t curDisp;
  uint8_t shift = y % 8;

  
  oled_setCursor (x, y);
  curDisp = dispBuffer[disp_row][disp_column];
  if (set)
    dispBuffer[disp_row][disp_column] = (curDisp | (0x01 << shift));
  else
    dispBuffer[disp_row][disp_column] = (curDisp & (~(0x01 << shift)));

}

void
oled_drawLine (float x1, float y1, float x2, float y2, bool set)
{
  // y=mx+c

  if (x1 == x2) // Vertical line has undefined slope;hence, plot without calculating the slope
    {
      for (uint8_t i = min(y1, y2); i <= max(y1, y2); i++)
	oled_setPixel (x1, i, set);

      return;
    }
  float slope = ((y2 - y1) / (x2 - x1));
  float c = y1 - slope * x1;

  for (float i = min(x1, x2); i <= max(x1, x2); i++)
    oled_setPixel (i, slope * i + c, set);

  for (float i = min(y1, y2); i <= max(y1, y2); i++)
    oled_setPixel ((i - c) / slope, i, set);
}

void
oled_drawRectangle (uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool set)
{

  oled_drawLine (x1, y1, x1, y2, set);

  oled_drawLine (x2, y1, x2, y2, set);

  oled_drawLine (x1, y1, x2, y1, set);

  oled_drawLine (x1, y2, x2, y2, set);

}

void
oled_plot (float current_x, float current_y)
{
  static uint8_t prev_x = 0;
  static float prev_y = 0;

  oled_drawLine (current_x, current_y, prev_x, prev_y, true);
  prev_y = current_y;
  prev_x = current_x;

  if (prev_x >= 127)
    {
      prev_x = 0;
    }
}

void
oled_drawCircle (float Cx, float Cy, float radius, bool set)
{
  //(x-cx)^2+(y-cy)^2=r^2

  for (uint8_t i = (Cx - radius); i <= (Cx + radius); i++)
    {
      oled_setPixel (i, (sqrt (sq(radius) - sq(i - Cx)) + Cy), set);
      oled_setPixel (i, (Cy - sqrt (sq(radius) - sq(i - Cx))), set);
    }

  for (uint8_t i = (Cy - radius); i <= (Cy + radius); i++)
    {
      oled_setPixel ((sqrt (sq(radius) - sq(i - Cy)) + Cx), i, set);
      oled_setPixel (Cx - (sqrt (sq(radius) - sq(i - Cy))), i, set);
    }
}

void
oled_displayBmp (const uint8_t binArray[])
{
  uint8_t line = 0, i;
  uint8_t chr;
  for (uint8_t y = 0; y < 64; y += 8)
    {

      oled_setCursor (0, y);

      for (uint8_t parts = 0; parts < 8; parts++)
	{

	  for (uint8_t column = 0; column < 16; column++)
	    {
	      chr = binArray[line * 128 + i];

	      dispBuffer[disp_row][disp_column++] = chr;

	      i++;
	    }
	}
      i = 0;
      line++;
    }
}

void
oled_resetLog ()
{
  log_row = 0;
  log_scroll = 0;
}

// Print text with auto scroll
void
oled_printLog (const char *log_msg)
{

  oled_clearPart (log_row >> 3, 0, 127);
  oled_printString (log_msg, 0, log_row, 6, false);
  log_row += 16;
  if (log_row == 64)
    {
      log_row = 0;
      if (!log_scroll)
	log_scroll = true;
    }

  if (log_scroll)
    {
      oled_setLineAddress (log_row);
    }
}

void
oled_drawSine (float frequency, uint8_t shift, bool set)
{

  for (float i = 0; i < 128; i += 1)

    oled_setPixel (i, 10 * sin (2 * PI * frequency * i) + shift, set);
}

long
map (long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void
oled_setBattery (uint8_t percentage)
{
  uint8_t temp;
  temp = map (percentage, 0, 100, 2, 12);
  for (uint8_t i = 2; i <= temp; i += 2)
    battery[i] = 0xFF;

  for (uint8_t i = temp; i < 11; i++)
    battery[i] = 0x81;
  if (percentage == 100)
    battery[12] = 0xFF;
  else
    battery[12] = 0xC3;

  oled_setCursor (110, 0);

  for (uint8_t i = 0; i < 14; i++)
    {

      dispBuffer[disp_row][disp_column++] = battery[i];

    }
}

void
oled_setSignal (uint8_t level)
{
  if (level == 0 || level < 20)
    {
      for (uint8_t i = 2; i <= 8; i += 2)
	signal[i] = 0x00;
    }
  else if (level >= 20 && level <= 50)
    {
      signal[2] = 0xC0;
      signal[4] = 0xF0;

      for (uint8_t i = 6; i <= 8; i += 2)
	signal[i] = 0x00;
    }
  else if (level > 50 && level <= 80)
    {
      signal[2] = 0xC0;
      signal[4] = 0xF0;
      signal[6] = 0xFC;
      signal[8] = 0x00;
    }
  else
    {
      signal[2] = 0xC0;
      signal[4] = 0xF0;
      signal[6] = 0xFC;
      signal[8] = 0xFF;
    }

  oled_setCursor (90, 0);

  for (uint8_t i = 0; i < 9; i++)
    {

      dispBuffer[disp_row][disp_column++] = signal[i];

    }

}

inline void
oled_barInit ()
{
  bar[0] = 0x3C;
  bar[1] = 0xC3;
  bar[126] = 0xC3;
  bar[127] = 0x3C;
}

void
oled_setBar (uint8_t level, uint8_t page)
{
  oled_barInit ();
  uint8_t temp = level;
  if (temp > 125)
    temp = 0;
  for (uint8_t i = 2; i <= temp; i++)
    bar[i] = 0xBD;

  for (uint8_t i = temp + 2; i <= 125; i++)
    bar[i] = 0x81;

  oled_setCursor (0, page * 8);
  for (uint8_t column = 0; column < 128; column++)
    dispBuffer[disp_row][disp_column++] = bar[column];

}
