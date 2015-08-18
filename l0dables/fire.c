/*
 * ----------------------------------------------------------------------------
 * "THE MATE-WARE LICENSE" (Revision 42): <leandro@tia.mat.br> wrote this
 * file.  As long as you retain this notice you can do whatever you want
 * with this stuff.  If we meet some day, and you think this stuff is worth
 * it, you can buy me a Club-Mate in return.               Leandro Pereira
 * ----------------------------------------------------------------------------
 */

#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/random.h>
#include <r0ketlib/render.h>

#include "usetable.h"

#undef RGB
#define RGB1(r,g,b) (((r)&0b11111000)|((g)>>5))
#define RGB2(r,g,b) (((g)&0b00011100)<<3|((b)>>3))
#define RGB(r,g,b) ((RGB1(r,g,b)<<8) | RGB2(r,g,b))

#define WIDTH 130
#define HEIGHT 30

static unsigned char fire[WIDTH * HEIGHT];
static unsigned char palette[256];

static void initPalette()
{
    int i;

    for (i = 0; i < 0x20; i++) {
	unsigned char double_i = i << 1;
	unsigned char quad_i = i << 2;
	unsigned char octo_i = i << 3;

	palette[i] = RGB(0, 0, double_i);
	palette[i + 32] = RGB(octo_i, 0, 64 - double_i);
	palette[i + 64] = RGB(255, octo_i, 0);
	palette[i + 96] = RGB(0xff, 0xff, quad_i);
	palette[i + 128] = RGB(0xff, 0xff, 64 + quad_i);
	palette[i + 160] = RGB(0xff, 0xff, 128 + quad_i);
	palette[i + 192] = RGB(0xff, 0xff, 192 + i);
	palette[i + 224] = RGB(0xff, 0xff, 224 + i);
    }
}

static void fireInit()
{
    int i, j;

    initPalette();

    for (i = WIDTH; i >= 0; i--) {
	for (j = 130 - HEIGHT; j >= 0; j--)
	    lcdSetPixel(i, j, 0);
    }
}

static void fireUpdate()
{
    int i;
    int j = WIDTH * (HEIGHT - 1);
    int k;

    for (i = 0; i < WIDTH - 1; i++)
	fire[j + i] = 0xff * (1 + (getRandom() & 0xf) > 10);

    for (k = 0; k < HEIGHT - 1; k++) {
	fire[j - WIDTH] = (fire[j] + fire[j + 1] + fire[j - WIDTH]) / 3;

	for (i = 1; i < WIDTH - 1; i++) {
	    short temp =
		(fire[j + i] + fire[j + i + 1] + fire[j + i - 1] +
		 fire[j - WIDTH + i]) >> 2;
	    fire[j - WIDTH + i] = temp - (temp > 1);
	}
	j -= WIDTH;
    }
}

void ram(void)
{
    fireInit();

    do {
	int i, j;

	lcdDisplay();

	fireUpdate();
	for (i = HEIGHT - 1; i >= 0; i--) {
	    for (j = WIDTH - 1; j >= 0; j--) {
		lcdSetPixel(j, i + 100, palette[fire[i * WIDTH + j]]);
	    }
	}
    } while (getInputRaw() == BTN_NONE);
}
