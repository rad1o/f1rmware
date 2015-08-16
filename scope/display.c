#include <stdint.h>

#include <rad1olib/pins.h>
#include <r0ketlib/print.h>
#include <r0ketlib/idle.h>
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#define TYPE_CMD    0
#define TYPE_DATA   1


void lcd_select() {
    /* the LCD requires 9-Bit frames */
    // Freq = PCLK / (CPSDVSR * [SCR+1])
	/* we want 120ns / bit -> 8.3 MHz. */
	/* SPI1 BASE CLOCK should be 40.8 MHz, CPSDVSR minimum is 2 */
	/* so we run at SCR=1 => =~ 10MHz */
    uint8_t serial_clock_rate = 1;
    uint8_t clock_prescale_rate = 2;

    ssp_init(LCD_SSP,
            SSP_DATA_9BITS,
            SSP_FRAME_SPI,
            SSP_CPOL_0_CPHA_0,
            serial_clock_rate,
            clock_prescale_rate,
            SSP_MODE_NORMAL,
            SSP_MASTER,
            SSP_SLAVE_OUT_ENABLE);

	OFF(LCD_CS);
}

void lcd_deselect() {
	ON(LCD_CS);
}

void lcdWrite(uint8_t cd, uint8_t data) {
    uint16_t frame = 0x0;

    frame = cd << 8;
    frame |= data;

	ssp_transfer(LCD_SSP, frame );
}


void startPixels(void) {
  // IMG_RAW16:
  lcd_select();
  lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,5);
  lcd_deselect();

  lcd_select();
  lcdWrite(TYPE_CMD,0x2C);
  lcd_deselect();

  lcd_select();
}

void emitPixel(uint8_t r, uint8_t g, uint8_t b) {
  r >>= 3;  // to 5 bit
  g >>= 2;  // to 6 bit
  b >>= 3;  // to 5 bit
  uint16_t pixel = (r << 11) | (g << 5) | b;

  lcdWrite(TYPE_DATA, pixel >> 8);
  lcdWrite(TYPE_DATA, pixel);
}

void stopPixels() {
  lcd_deselect();
}

// STUB
void lcdSetContrast() {
}

// STUB
void lcdSetRotation() {
}
