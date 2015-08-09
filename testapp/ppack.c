/*
 * Copyright 2015 team rad1o
 *
 */

#include <unistd.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <r0ketlib/fs_util.h>
#include <rad1olib/pins.h>

#include <portapack_hackrf/portapack.h>

//# MENU aappack
void ppack_menu() {
	lcdClear();
	lcdPrintln("PPack port");
	lcdPrintln("up:   ");
	lcdPrintln("down: ");
	lcdPrintln("l/r:  ");
	lcdDisplay();

	while(1){
		switch(getInputWait()){
			case BTN_UP:
			    ON(EN_VDD);
			    // ON(MIXER_EN); // hackrf does this
			    OFF(MIC_AMP_DIS);
			    cpu_clock_set(204); // WARP SPEED! :-)

			    break;
			case BTN_DOWN:
				break;
			case BTN_LEFT:
				break;
			case BTN_RIGHT:
				break;
			case BTN_ENTER:
				return;
		};
	};
};
