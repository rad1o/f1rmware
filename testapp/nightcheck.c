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
#include <r0ketlib/print.h>
#include <r0ketlib/night.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

//# MENU night
void menu_night(void){
    while(getInputRaw()!=BTN_ENTER){
        lcdClear();
        lcdPrint("night:");
        lcdPrint(IntToStr(isNight(),3,0));
        lcdNl();
        lcdPrint("light:");
        lcdPrint(IntToStr(GetLight(),3,0));
        lcdNl();
        lcdDisplay();
    };
}

