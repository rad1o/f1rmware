#include <rad1olib/light_ws2812_cortex.h>
#include <r0ketlib/fs_util.h>

#define MAX_LED_FRAMES 50

unsigned char leds[3*8*MAX_LED_FRAMES];
unsigned int frames = 0;

void rgbLedsInit(void) {
	char filename[] = "rgb_leds.hex";
	int size = getFileSize(filename);
	if(size > 0) {
		if(size >= 3*8*MAX_LED_FRAMES)
			size = 3*8*MAX_LED_FRAMES;
		readFile(filename, (char*)leds, size);
		frames = (size-1)/(3*8);
	}
}

void rgbLedsTick(void) {
	static unsigned int ledctr = 0;
	static unsigned int ctr = 0;
	ctr++;

	if(frames > 0) {
		// LED delay is in leds[0]
		if(ctr >= leds[0]){
			ws2812_sendarray(&leds[ledctr*3*8+1], 3*8);
			ledctr++;
			if(ledctr >= frames)
				ledctr = 0;

			ctr = 0;
		}
	}

	return;
}
