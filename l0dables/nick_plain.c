#include <stdint.h>
#include <string.h>

#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/config.h>
#include <r0ketlib/render.h>

#include "usetable.h"

void ram(void) {
  int dx=0;
  int dy=0;
    static uint32_t ctr=0;
  ctr++;

  setExtFont(GLOBAL(nickfont));
  dx=DoString(0,0,GLOBAL(nickname));
    dx=(RESX-dx)/2;
    if(dx<0)
        dx=0;
    dy=(RESY-getFontHeight())/2;

  lcdClear();
  lcdFill(GLOBAL(nickbg));
  setTextColor(GLOBAL(nickbg),GLOBAL(nickfg));
  lcdSetCrsr(dx,dy);
  lcdPrint(GLOBAL(nickname));
  lcdDisplay();

  getInputWait();
  setTextColor(0xFF,0x00);
  return;
  
}
