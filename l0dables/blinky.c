/*
 * Copyright 2015 team rad1o
 *
 * This is a small sample for a l0dable
 *
 */

#include <libopencm3/lpc43xx/gpio.h>
#include <rad1olib/pins.h>
#include <rad1olib/setup.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include "usetable.h"

void ram(void){
	lcdPrintln("l0dable-Test");
	lcdDisplay();

	lcdNl();
	lcdDisplay();
	while(1){
	    lcdSetCrsr(0,8*8);
	    lcdPrint("ctr=");
	    lcdPrint(IntToStr(_timectr/1000,5,F_LONG));
	    lcdDisplay();
	    switch(getInputRaw()){
		case BTN_ENTER:
		    return;
		case BTN_LEFT:
		    TOGGLE(RAD1O_LED3);
		    break;
		case BTN_RIGHT:
		    TOGGLE(RAD1O_LED3);
		    delayNop(10000000);
		    break;
		case BTN_UP:
		    ON(RAD1O_LED3);
		    break;
		case BTN_DOWN:
		    OFF(RAD1O_LED3);
		    break;
	    };
	}
	getInputWaitRelease();
	return;
}
