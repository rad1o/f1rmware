#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

void ws1812b_animation(void)
{
	const uint8_t nleds = 8;
	uint8_t pattern[nleds * 3];

	const uint16_t hue_max = 767;
	uint16_t base_hue = 0, hue = 0;

	lcdPrintln("Animating...");
	lcdPrintln("Any key to exit");
	lcdDisplay();
	while(!getInput())
	{
		for(uint8_t nled = 0; nled < nleds; nled++)
		{
			// distribute the complete 16 bit hue amongh all leds
			hue = base_hue + (nled * (hue_max / nleds));
			hsl2rgb(hue, 255, 255, &pattern[nled * 3]);
		}

		//hsl2rgb(base_hue, 255, 255, &pattern[0]);
		ws2812_sendarray(pattern, sizeof(pattern));

		if(++base_hue == hue_max) base_hue = 0;
		delayNop(100000);
	}
}

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
	SETUPgout(RAD1O_RGB_LED);

	while(1){
		lcdClear(0xff);
		lcdPrintln("WS2812B test");
		lcdPrintln("UP: pattern");
		lcdPrintln("DOWN: green");
		lcdPrintln("RIGHT: animation");
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
				ws1812b_animation();
				break;
			case BTN_ENTER:
				return;
				break;
		};
	};
	return;
};
