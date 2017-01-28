#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#include <libopencm3/lpc43xx/adc.h>

//# MENU led
void led_menu(){
	getInputWaitRelease();

	uint8_t leds=0;
	uint8_t adc=0;
	while(1){
		lcdClear();
		lcdPrintln("LED:");
		lcdPrintln("");
		lcdPrintln("U Toggle LEDs");
		lcdPrintln("D Toggle ADC");
		lcdPrintln("");
		lcdDisplay();
		switch(getInput()){
			case BTN_UP:
				adc=0;
				leds=1-leds;
				SETUPgout(RAD1O_LED4);
				if(leds){
					ON(RAD1O_LED1);
					ON(RAD1O_LED2);
					ON(RAD1O_LED3);
					ON(RAD1O_LED4);
				}else{
					OFF(RAD1O_LED1);
					OFF(RAD1O_LED2);
					OFF(RAD1O_LED3);
					OFF(RAD1O_LED4);
				};
				break;
			case BTN_DOWN:
				adc=1;
				SETUPadc(RAD1O_LED4);
				break;
			case BTN_ENTER:
				SETUPgout(RAD1O_LED1);
				SETUPgout(RAD1O_LED2);
				SETUPgout(RAD1O_LED3);
				SETUPgout(RAD1O_LED4);
				return;
		};
		if(adc){
			lcdPrint("LED4: ");
			lcdPrint(IntToStr(adc_get_single(ADC0,ADC_CR_CH6)*2*330/1023,4,F_LONG));
			lcdNl();
			lcdDisplay();
		};
		getInputWaitRelease();
	};
};
