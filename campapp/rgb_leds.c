#include <rad1olib/light_ws2812_cortex.h>
#include <r0ketlib/fs_util.h>

#define MAX_LED_FRAMES 50
#define BUF_SIZE 3*8*MAX_LED_FRAMES+2

unsigned char leds[BUF_SIZE];
unsigned int frames = 0;

void rgbLedsInit(void) {
	char filename[] = "rgb_leds.hex";
	int size = getFileSize(filename);
	if(size > 0) {
		if(size >= BUF_SIZE)
			size = BUF_SIZE;
		readFile(filename, (char*)leds, size);
		frames = (size-1)/(3*8);
	}
}

void rgbLedsTick(void) {
	static unsigned int ledctr = 0;
	static unsigned int ctr = 0;
	ctr++;

	if(frames > 0) {
		// LED delay is in leds[0:1]
		if(ctr >= ((leds[0]<<8) + leds[1])){
			ws2812_sendarray(&leds[ledctr*3*8+2], 3*8);
			ledctr++;
			if(ledctr >= frames)
				ledctr = 0;

			ctr = 0;
		}
	}

	return;
}
