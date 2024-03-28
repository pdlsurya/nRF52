#ifndef __OLED_SH1106_H
#define __OLED_SH1106_H


#include "stdint.h"
#include "stdbool.h"


void oled_init();

void oled_clearDisplay();

void oled_clearPart(uint8_t page, uint8_t start_pos, uint8_t end_pos);

void oled_setCursorPos(uint8_t x, uint8_t y);

void oled_displayBmp(const uint8_t binArray[]);

void oled_printChar(char C, uint8_t xpos, uint8_t ypos, uint8_t font_size, bool Highlight);

void oled_printString(const char *str, uint8_t xpos, uint8_t ypos, uint8_t font_size, bool Highlight);

void oled_setBattery(uint8_t percentage);

void oled_setSignal(uint8_t level);

void oled_setBar(uint8_t level, uint8_t page);

void oled_display();

void oled_setCursor(uint8_t x, uint8_t y);

void oled_print7Seg_number(const char *str, uint8_t xpos, uint8_t ypos);

void oled_print7Seg_digit(char C, uint8_t xpos, uint8_t ypos);

void oled_writeByte(uint8_t byte);

void oled_setPixel(uint8_t x, uint8_t y, bool set);

void oled_drawLine(float x1, float y1, float x2, float y2, bool set);

void oled_drawRectangle(uint8_t x1,uint8_t y1, uint8_t x2, uint8_t y2, bool set);

void oled_drawCircle(float Cx, float Cy, float radius, bool set);

void oled_setDisplayStart(uint8_t Start);

void oled_setLineAddress(uint8_t address);

void oled_printLog(const char *log_msg);

void oled_drawSine(float frequency, uint8_t shift, bool set);

void oled_plot(float x, float y);

void oled_setContrast(uint8_t level);

void oled_resetLog();

long map(long x, long in_min, long in_max, long out_min, long out_max);

#endif
