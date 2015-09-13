#include <r0ketlib/config.h>
#include <r0ketlib/select.h>
#include <r0ketlib/print.h>
#include <r0ketlib/display.h>
#include <rad1olib/light_ws2812_cortex.h>
#include <r0ketlib/fs_util.h>
#include <r0ketlib/keyin.h>
#include <string.h>
#include <campapp/rad1oconfig.h>

#define MAX_LED_FRAMES 50
#define BUF_SIZE 3*8*MAX_LED_FRAMES+2

unsigned char leds[BUF_SIZE];
unsigned int frames = 0;
unsigned int ctr = 0;
unsigned int framectr = 0;

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
				ws2812_sendarray(&leds[framectr*3*8+2], 3*8);
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
