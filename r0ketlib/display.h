#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <libopencm3/cm3/common.h>

#define RESX 132
#define RESY 132

/* Display buffer */
extern uint8_t lcdBuffer[RESX*RESY];

void lcdInit(void);
void lcdFill(char f);
void lcdDisplay(void);
void lcdSetPixel(char x, char y, uint8_t f);
uint8_t lcdGetPixel(char x, char y);
void lcdShift(int x, int y, bool wrap);
void lcdSetContrast(int c);
#endif
