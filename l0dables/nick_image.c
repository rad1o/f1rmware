#include <r0ketlib/config.h>
#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/image.h>

#include "usetable.h"

void ram(void){
  if (lcdShowImageFile("nick.lcd") != 0) {
    lcdClear();
    lcdNl();
    lcdPrintln("-------------------");
    lcdPrintln("-   Image file    -");
    lcdPrintln("-    nick.lcd     -");
    lcdPrintln("-   not found.    -");
    lcdPrintln("-                 -");
    lcdPrintln("- Copy it here or -");
    lcdPrintln("- choose another  -");
    lcdPrintln("-    animation.   -");
    lcdPrintln("-------------------");
    lcdNl();
    lcdDisplay();
  }
}
