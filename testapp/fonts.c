#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/fonts/orbitron14.h>
#include <r0ketlib/fonts/ubuntu18.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/render.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>

const char * txt="The quick brown fx";

//# MENU fonts
void fonts_menu(){
    getInputWaitRelease();
    setSystemFont();
    lcdClear();
    lcdPrintln("Font test.");
    lcdDisplay();

    while(1){
	switch(getInputWait()){
	    case BTN_UP:
		setIntFont(&Font_Orbitron14pt);
		break;
	    case BTN_DOWN:
		setIntFont(&Font_Ubuntu18pt);
		break;
	    case BTN_LEFT:
		setIntFont(&Font_8x8);
		break;
	    case BTN_RIGHT:
		setIntFont(&Font_8x8Thin);
		break;
	    case BTN_ENTER:
		return;
	};
	lcdPrintln(txt);
	lcdDisplay();
	getInputWaitRelease();
    };
};
