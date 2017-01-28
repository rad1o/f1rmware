/* nick_ledrainbow.c
 * Plain nick animation that shows a moving rainbow on the rad1o's antenna
 * (needs the ws2812 leds populated)
 *
 * By Nervengift
 * rad1o@nerven.gift
 *
 * Known bugs:
 * - openingg the main menu does not always work at first try
 */
#include <r0ketlib/display.h>
#include <r0ketlib/config.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#include "usetable.h"

#define DIM 0.1

typedef struct {
	uint8_t r, g, b;
} Color;

Color wheel(uint8_t pos);

void ram(){
	uint8_t pattern[24];
	int offset = 0;
	int dx=0;
	int dy=0;

	uint8_t color[] = {0, 0, 0};
	Color col;
	SETUPgout(RAD1O_RGB_LED);

	setExtFont(GLOBAL(nickfont));
	setTextColor(GLOBAL(nickbg),GLOBAL(nickfg));
	dx=DoString(0,0,GLOBAL(nickname));
	lcdFill(GLOBAL(nickbg));
	dx=(RESX-dx)/2;
	if(dx<0)
		dx=0;
	dy=(RESY-getFontHeight())/2;
	lcdSetCrsr(dx,dy);
	lcdPrint(GLOBAL(nickname));
	lcdDisplay();

	offset=0;
	pattern[0] = 55 * DIM;
	pattern[1] = 0 * DIM;
	pattern[2] = 55 * DIM;
	pattern[3] = 55 * DIM;
	pattern[4] = 0 * DIM;
	pattern[5] = 55 * DIM;
	while (1) {
		for (int i = 0; i < 6; i++) {
			col=wheel((-offset+30 * i)%255);
			pattern[3*i+0+6] = col.g * DIM;
			pattern[3*i+1+6] = col.r * DIM;
			pattern[3*i+2+6] = col.b * DIM;
		}
		ws2812_sendarray(pattern, sizeof(pattern));
		offset+=3;
		if(getInput() != BTN_NONE) return;
		delayms_queue_plus(50, 0);
	}
	return;
};

Color wheel(uint8_t pos) {
	Color c;
	if (pos < 85) {
		c = (Color) {pos * 3, 255 - pos*3, 0};
	} else if (pos < 170) {
		pos -= 85;
		c = (Color) {255 - pos*3, 0, pos * 3};
	} else {
		pos -= 170;
		c = (Color) {0, pos * 3, 255 - pos*3};
	}
	return c;
}
