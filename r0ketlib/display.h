#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <libopencm3/cm3/common.h>

#define RESX 130
#define RESY 130

#define TYPE_CMD    0
#define TYPE_DATA   1

#define RGB(rgb) (rgb & 0b111000000000000000000000) >> 16 | (rgb & 0b000000001110000000000000) >> 11 | (rgb & 0b000000000000000011000000) >> 6

/* Display buffer */
extern uint8_t lcdBuffer[RESX*RESY];

void lcdInit(void);
void lcdFill(char f);
void lcdDisplay(void);
void lcdSetPixel(char x, char y, uint8_t f);
uint8_t lcdGetPixel(char x, char y);
void lcdShift(int x, int y, int wrap);
void lcdSetContrast(int c);
void lcdSetRotation(char doit);
void lcdRotate(void);

void lcd_select();
void lcd_deselect();
void lcdWrite(uint8_t cd, uint8_t data);

#define RGB_TO_8BIT(r, g, b)                                    \
  ((r & 0b11100000) | ((g >> 3) & 0b11100) | ((b >> 6) & 0b11))

#endif
