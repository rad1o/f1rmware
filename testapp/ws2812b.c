#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

//# MENU ws2812b
void ws1812b_menu(){
    uint8_t pattern[] = {
                     255, 255, 0,
                     255, 255, 0,

                     0,   0,   255,
                     0,   0,   255,
                     0,   0,   255,
                     0,   0,   255,
                     0,   0,   255,
                     255, 0,   0
                     };

    uint8_t green[] = {255, 0, 0};
	getInputWaitRelease();
    //cpu_clock_set(17);
    cpu_clock_set(51);
	SETUPgout(RGB_LED);

	while(1){
		lcdClear(0xff);
		lcdPrintln("WS2812B test");
		lcdPrintln("UP: pattern");
		lcdPrintln("DOWN: green");
		lcdPrintln("ENTER: exit");
		lcdDisplay();

		switch(getInput()){
			case BTN_UP:
                ws2812_sendarray(pattern, sizeof(pattern));
				break;
			case BTN_DOWN:
                ws2812_sendarray(green, sizeof(green));
				break;
			case BTN_LEFT:
				break;
			case BTN_RIGHT:
				break;
			case BTN_ENTER:
				return;
				break;
		};
	};
	return;
};
