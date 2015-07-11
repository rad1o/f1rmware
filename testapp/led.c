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
				SETUPgout(LED4);
				if(leds){
					ON(LED1);
					ON(LED2);
					ON(LED3);
					ON(LED4);
				}else{
					OFF(LED1);
					OFF(LED2);
					OFF(LED3);
					OFF(LED4);
				};
				break;
			case BTN_DOWN:
				adc=1;
				SETUPadc(LED4);
				break;
			case BTN_ENTER:
				SETUPgout(LED1);
				SETUPgout(LED2);
				SETUPgout(LED3);
				SETUPgout(LED4);
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
