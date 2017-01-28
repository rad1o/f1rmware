#include <r0ketlib/config.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/render.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/select.h>
#include <r0ketlib/stringin.h>
#include <r0ketlib/image.h>
#include <rad1olib/pins.h>
#include <rad1olib/systick.h>
#include <lpcapi/msc/msc_main.h>

/**************************************************************************/

void msc_menu(void){
    MSCenable();
    lcdPrintln("MSC enabled.");
    lcdDisplay();
    getInputWaitRelease();
    getInputWait();
    lcdPrintln("disconnect");
    lcdDisplay();
    MSCdisable();
    fsReInit();
    getInputWaitRelease();
}

void tick_alive(void){
    static int foo=0;

    if(GLOBAL(alivechk)==0)
        return;

	if(foo++>500/SYSTICKSPEED){
		foo=0;
        TOGGLE(RAD1O_LED2);
	};
    return;
}

//# MENU image display_image
void t_img(void){
    char fname[FLEN];
    selectFile(fname, "LCD");
    lcdShowImageFile(fname);
    getInputWait();
}

//# MENU image play_animation
void t_ani(void){
    char fname[FLEN];
    selectFile(fname, "AN1");
    lcdShowAnim(fname);
}

void infoscreen();
//# MENU INFO
void night(void){
    infoscreen();
}
