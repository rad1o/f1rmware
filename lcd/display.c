/* for memmove etc.*/
#include <string.h>

#include "display.h"
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>



/**************************************************************************/
/* Utility routines to manage nokia display */
/**************************************************************************/

uint8_t lcdBuffer[RESX*RESY];
uint8_t displayType;

#define TYPE_CMD    0
#define TYPE_DATA   1

void delayms(uint32_t duration){ /* XXX: do me correctly */
	uint32_t i;

	duration*=10000;

	for (i = 0; i < duration; i++)
		__asm__("nop");
}


static void lcd_select() {
    /* the LCD requires 9-Bit frames */
	// XXX: make correct freqency
    // Freq About 0.0498MHz / 49.8KHz => Freq = PCLK / (CPSDVSR * [SCR+1]) with PCLK=PLL1=204MHz
    uint8_t serial_clock_rate = 21;
    uint8_t clock_prescale_rate = 2;

    ssp_init(SSP1_NUM,
            SSP_DATA_9BITS,
            SSP_FRAME_SPI,
            SSP_CPOL_0_CPHA_0,
            serial_clock_rate,
            clock_prescale_rate,
            SSP_MODE_NORMAL,
            SSP_MASTER,
            SSP_SLAVE_OUT_ENABLE);
	
	gpio_clear(LCD_CS_GPORT,LCD_CS_GPIN);
}

static void lcd_deselect() {
	gpio_set(LCD_CS_GPORT,LCD_CS_GPIN);
}

static void lcdWrite(uint8_t cd, uint8_t data) {
    uint16_t frame = 0x0;

    frame = cd << 8;
    frame |= data;

	ssp_transfer(LCD_SSP, frame );
}

void lcdInit(void) {
	scu_pinmux(LCD_BL_PIN,SCU_GPIO_NOPULL|LCD_BL_FUNC);
	GPIO_DIR(LCD_BL_GPORT) |= LCD_BL_GPIN;
	gpio_set(LCD_BL_GPORT,LCD_BL_GPIN);

	scu_pinmux(LCD_RESET_PIN,SCU_GPIO_NOPULL|LCD_RESET_FUNC);
	GPIO_DIR(LCD_RESET_GPORT) |= LCD_RESET_GPIN;

	scu_pinmux(LCD_CS_PIN,SCU_GPIO_NOPULL|LCD_CS_FUNC);
	GPIO_DIR(LCD_CS_GPORT) |= LCD_CS_GPIN;

	scu_pinmux(LCD_MOSI_PIN,LCD_MOSI_FUNC);
	scu_pinmux(LCD_SCK_PIN,LCD_SCK_FUNC);

	// Reset the display
    delayms(100);
    gpio_clear(LCD_RESET_GPORT,LCD_RESET_GPORT);
    delayms(100);
    gpio_set(LCD_RESET_GPORT,LCD_RESET_GPORT);
    delayms(100);

    lcd_select();

	static uint8_t initseq_d[] = {
		/* The controller is a PCF8833 -
		   documentation can be found online.
		 */
		0x11, 
		0x3A, 2, 
//		0x36, 0x60, 
		0x36, 0b11000000, // my,mx,v,lao,rgb,x,x,x
		0x25, 0x3a, 
		0x29,
		0x03,
		0x2A, 0, RESX-1, 
		0x2B, 0, RESY-1
	};
	uint16_t initseq_c = ~  (  /* commands: 1, data: 0 */
			(1<< 0) |
			(1<< 1) | (0<< 2) |
			(1<< 3) | (0<< 4) |
			(1<< 5) | (0<< 6) |
			(1<< 7) |
			(1<< 8) |
			(1<< 9) | (0<<10) | (0<<11) |
			(1<<12) | (0<<13) | (0<<14) |
			0);
	int i = 0;

	lcdWrite(0, 0x01); /* most color displays need the pause */
	delayms(10);

	while(i<sizeof(initseq_d)){
		lcdWrite(initseq_c&1, initseq_d[i++]);
		initseq_c = initseq_c >> 1;
	}
    lcd_deselect();
}

void lcdFill(char f){
    memset(lcdBuffer,f,RESX*RESY);
#if 0
    int x;
    for(x=0;x<RESX*RESY;x++) {
        lcdBuffer[x]=f;
    }
#endif
}

void lcdSetPixel(char x, char y, uint8_t f){
    if (x<0 || x> RESX || y<0 || y > RESY)
        return;
    lcdBuffer[y*RESX+x] = f;
}

uint8_t lcdGetPixel(char x, char y){
    return lcdBuffer[y*RESX+x];
}

void lcdDisplay(void) {
    lcd_select();

	uint16_t x,y;

	lcdWrite(TYPE_CMD,0x2C);

	for(y=0;y<RESY;y++){
		for(x=0;x<RESX;x++){
			lcdWrite(TYPE_DATA,lcdGetPixel(x,y)); 
		};
	}
    lcd_deselect();
}

void lcdSetContrast(int c) {
    lcd_select();
	lcdWrite(TYPE_CMD,0x25);
	lcdWrite(TYPE_DATA,c);
    lcd_deselect();
}

void lcdShiftH(bool right, bool wrap) {
	uint8_t tmp;
	for (int yb = 0; yb<RESY; yb++) {
		if (right) {
			tmp = lcdBuffer[yb*RESX];
			memmove(lcdBuffer + yb*RESX,lcdBuffer + yb*RESX+1 ,RESX-1);
            lcdBuffer[yb*RESX+(RESX-1)] = wrap?tmp:0xff;
		} else {
			tmp = lcdBuffer[yb*RESX+(RESX-1)];
			memmove(lcdBuffer + yb*RESX+1,lcdBuffer + yb*RESX ,RESX-1);
			lcdBuffer[yb*RESX] = wrap?tmp:0xff;
		}
	}
}

void lcdShiftV(bool up, bool wrap) {
	uint8_t tmp[RESX];
	if (up) {
		if (wrap)
            memmove(tmp, lcdBuffer, RESX);
        else
            memset(tmp,0xff,RESX);
		memmove(lcdBuffer,lcdBuffer+RESX ,RESX*(RESY-1));
		memmove(lcdBuffer+RESX*(RESY-1),tmp,RESX);
	} else {
		if (wrap)
            memmove(tmp, lcdBuffer+RESX*(RESY-1), RESX);
        else
            memset(tmp,0xff,RESX);
		memmove(lcdBuffer+RESX,lcdBuffer ,RESX*(RESY-1));
		memmove(lcdBuffer,tmp,RESX);
	}
}

void lcdShift(int x, int y, bool wrap) {
	bool dir=true;

    if(x<0){
        dir=false;
        x=-x;
    };

    while(x-->0)
        lcdShiftH(dir, wrap);

    if(y<0){
        dir=false;
        y=-y;
    }else{
        dir=true;
    };

    while(y-->0)
        lcdShiftV(dir, wrap);
}

