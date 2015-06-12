#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <libopencm3/cm3/common.h>

#define RESX 132
#define RESY 132

/* XXX: probably should be in central header file */
#define LCD_CS_PIN     (P9_0)
#define LCD_CS_FUNC    (SCU_CONF_FUNCTION0)
#define LCD_CS_GPORT   GPIO4
#define LCD_CS_GPIN    GPIOPIN12

#define LCD_RESET_PIN  (P9_4)
#define LCD_RESET_FUNC (SCU_CONF_FUNCTION4)
#define LCD_RESET_GPORT (GPIO5)
#define LCD_RESET_GPIN (GPIOPIN17)

#define LCD_MOSI_PIN   (P1_4)
#define LCD_MOSI_FUNC  (SCU_SSP_IO | SCU_CONF_FUNCTION5)

#define LCD_SCK_PIN    (P1_19)
#define LCD_SCK_FUNC   (SCU_SSP_IO | SCU_CONF_FUNCTION1)

#define LCD_SSP (SSP1_NUM)

#define LCD_BL_PIN (P1_1)
#define LCD_BL_FUNC (SCU_CONF_FUNCTION0)
#define LCD_BL_GPORT GPIO0
#define LCD_BL_GPIN GPIOPIN8

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
