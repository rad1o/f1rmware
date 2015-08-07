/*
 * This l0dable by Benedikt Roth and Stefan Tomanek serves as your main
 * viewscreen, displaying your nickname the way you want it, and lets the
 * stars zoom by. You can accelerate your vessel by pushing the joystick
 * upwards or bring it to a halt by pressing it down - leaving your ship
 * drifting in the endless space. Attach two LEDs to the modulbus
 * connectors (SS2->GND, SS5->GND), so your r0ket can light up its nacelles
 * when breaking the warp barrier.
 *
 * commit 33fe346942176a0e988818980d04d1a8f746f894 1 parent 0eaf74fa87
 * wertarbyte authored August 13, 2011
 */
#include <sysinit.h>

#include "basic/basic.h"
#include "basic/config.h"

#include "lcd/lcd.h"
#include "lcd/print.h"

#include "usetable.h"

#define NUM_STARS 100
#define SPEED_MAX 10
#define SPEED_DEFAULT 4
#define SPEED_STOP 0
#define SPEED_WARP 6

// Two RGB LEDs on the Modulbus
#define LEDA_R RB_SPI_SS0
#define LEDA_G RB_SPI_SS1
#define LEDA_B RB_SPI_SS2

#define LEDB_R RB_SPI_SS3
#define LEDB_G RB_SPI_SS4
#define LEDB_B RB_SPI_SS5

typedef struct {
	short x, y, z;
} s_star;

typedef struct {
	short speed;
} s_ship;

static s_ship ship = {SPEED_DEFAULT};

static s_star stars[NUM_STARS];

void init_star(s_star *star, int z);
void set_warp_lights(uint8_t enabled);
void drift_ship(void);

void ram(void)
{
	short centerx = RESX >> 1;
	short centery = RESY >> 1;
	short i;
	uint8_t key = 0;

	for (i = 0; i < NUM_STARS; i++) {
		init_star(stars + i, i + 1);
	}

	static uint8_t count = 0;
	while(1) {
		count++;
		count%=256;
		key = getInputRaw();
		if (key == BTN_ENTER) {
			break;
		} else if ( count%4 == 0 ) {
			if (key == BTN_UP && ship.speed < SPEED_MAX) {
				ship.speed++;
			} else if (key == BTN_DOWN && ship.speed > SPEED_STOP) {
				ship.speed--;
			} else if (key ==BTN_NONE) {
				/* converge towards default speed */
				if (ship.speed < SPEED_DEFAULT) {
					ship.speed++;
				} else if (ship.speed > SPEED_DEFAULT) {
					ship.speed--;
				}
			}
		}

		if (ship.speed > SPEED_WARP) {
			set_warp_lights(1);
		} else {
			set_warp_lights(0);
		}

		if (ship.speed == 0 && count%6==0) drift_ship();

		int dx=0;
		int dy=0;
		setExtFont(GLOBAL(nickfont));
		dx=DoString(0,0,GLOBAL(nickname));
		dx=(RESX-dx)/2;
		if(dx<0) dx=0;
		dy=(RESY-getFontHeight())/2;

		lcdClear();
		DoString(dx,dy,GLOBAL(nickname));

		for (i = 0; i < NUM_STARS; i++) {
			stars[i].z -= ship.speed;

			if (ship.speed > 0 && stars[i].z <= 0)
				init_star(stars + i, i + 1);

			short tempx = ((stars[i].x * 30) / stars[i].z) + centerx;
			short tempy = ((stars[i].y * 30) / stars[i].z) + centery;

			if (tempx < 0 || tempx > RESX - 1 || tempy < 0 || tempy > RESY - 1) {
				if (ship.speed > 0) { /* if we are flying, generate new stars in front */
					init_star(stars + i, i + 1);
				} else { /* if we are drifting, simply move those stars to the other end */
					stars[i].x = (((tempx%RESX)-centerx)*stars[i].z)/30;
					stars[i].y = (((tempy%RESY)-centery)*stars[i].z)/30;
				}
				continue;
			}

			lcdSetPixel(tempx, tempy, 1);
			if (stars[i].z < 50) {
				lcdSetPixel(tempx + 1, tempy, 1);
			}
			if (stars[i].z < 20) {
				lcdSetPixel(tempx, tempy + 1, 1);
				lcdSetPixel(tempx + 1, tempy + 1, 1);
			}
		}

		lcdRefresh();

		delayms_queue_plus(50,0);
	}
	set_warp_lights(0);
}

void set_warp_lights(uint8_t enabled) {
	gpioSetValue(LEDA_R, 0);
	gpioSetValue(LEDA_G, 0);
	gpioSetValue(LEDA_B, enabled);

	gpioSetValue(LEDB_R, 0);
	gpioSetValue(LEDB_G, 0);
	gpioSetValue(LEDB_B, enabled);
}

void drift_ship(void) {
	uint8_t d_x = 1;
	uint8_t d_y = 1;
	for (uint8_t i = 0; i < NUM_STARS; i++) {
		stars[i].x += d_x;
		stars[i].y += d_y;
	}
}

void init_star(s_star *star, int z)
{
	star->x = (getRandom() % RESX) - (RESX >> 1);
	star->y = (getRandom() % RESY) - (RESY >> 1);
	star->z = z;

	return;
}

