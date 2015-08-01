#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <lpcapi/msc/msc_main.h>
#include <libopencmsis/core_cm3.h>

#include <r0ketlib/fs_util.h>

//# MENU MSC
void msc_menu(){
	MSCenable();
	lcdPrintln("MSC enabled.");
	getInputWaitRelease();
	while(getInputRaw()!=BTN_ENTER){
		if(getInputRaw()==BTN_RIGHT)
			lcdPrintln(".");
		lcdDisplay();
		__WFI();
	};
	lcdPrintln("disconnect");
	lcdDisplay();
	MSCdisable();
	fsReInit();
	getInputWaitRelease();
};
