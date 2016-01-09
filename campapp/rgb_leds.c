#include <r0ketlib/config.h>
#include <r0ketlib/select.h>
#include <r0ketlib/print.h>
#include <r0ketlib/display.h>
#include <rad1olib/light_ws2812_cortex.h>
#include <r0ketlib/fs_util.h>
#include <r0ketlib/keyin.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <campapp/rad1oconfig.h>

#define MAX_LED_FRAMES 50
#define BUF_SIZE 3*8*MAX_LED_FRAMES+2

uint8_t leds[BUF_SIZE];
uint8_t frames = 0;
uint16_t ctr = 0;
uint8_t framectr = 0;

void readRgbLedFile(void) {
	int size = getFileSize(GLOBAL(ledfile));
	frames = 0;
	ctr = 0;
	framectr = 0;
	if(size > 0) {
		if(size >= BUF_SIZE)
			size = BUF_SIZE;
		readFile(GLOBAL(ledfile), (char*)leds, size);
		frames = (size-2)/(3*8);
	}
}

/**************************************************************************/

void init_rgbLeds(void) {
	readTextFile("ledfile.cfg",GLOBAL(ledfile),FLEN);
	readRgbLedFile();
}

void tick_rgbLeds(void) {
	if(GLOBAL(rgbleds)) {
		if(frames > 0) {
			if(ctr == 0) {
				uint8_t amplified[3*8];

				// determine amplifier level based on the config
				int8_t amplvl = GLOBAL(rgbleds_amp) - 8;
				if (amplvl > 0) {
					amplvl = round(sqrt(pow(2,amplvl+1)));
				} else if (amplvl < 0) {
					amplvl = -round(sqrt(pow(2,-amplvl+1)));
				} else {
					amplvl = 1;
				}

				// iterate through every value in the frame (3 channels * 8 LEDs)
				for (int8_t i=0; i<3*8; i++) {
					// copy original value
					uint8_t origval = leds[(framectr*3*8+2)+i];
					// set the amplified value
					if (amplvl >= 0) {
						amplified[i] = origval * amplvl;
						if (amplvl != 0 && amplified[i] / amplvl != origval) {
							// overflow!
							amplified[i] = 255;
						}
					} else {
						amplified[i] = origval / -amplvl;
					}
				}

				ws2812_sendarray(&amplified[0], 3*8);
				framectr++;
				if(framectr >= frames)
					framectr = 0;
			}

			ctr++;
			// LED delay is in leds[0:1]
			if(ctr >= ((leds[0]<<8) + leds[1]))
				ctr = 0;
		}
	}
	return;
}

//# MENU rgb_leds
void selectLedFile(void){
    if(GLOBAL(rgbleds)) {
        if(init_selectFile("L3D")){
            while(selectFileRepeat(GLOBAL(ledfile),"L3D") >= 0) {
                writeFile("ledfile.cfg", GLOBAL(ledfile), strlen(GLOBAL(ledfile)));
                init_rgbLeds();
            }
        }
    } else {
        lcdClear();
        lcdNl();
        lcdPrintln("You need to enable");
        lcdPrintln("<rgbleds> in the");
        lcdPrintln("config to use");
        lcdPrintln("this!");
        lcdNl();
        lcdPrintln(" LEFT: back");
        lcdPrintln(" ENTER/RIGHT:");
        lcdPrintln("       open config");
        lcdDisplay();

        while(1){
            switch(getInput()){
                case BTN_LEFT:
                    return;
                case BTN_RIGHT:
                case BTN_ENTER:
                    menu_config();
                    return;
            }
        }
    }
}
