
#include <r0ketlib/config.h>
#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>

#include "usetable.h"

#define one ((uint32_t)1)

typedef uint8_t uchar;

static unsigned long iter=0;

void ram(void) {
    getInputWaitRelease();

    char template[256];
    static int nickx=2,nicky=10;
    static int nickwidth,nickheight;
    static int nickoff=10;
    static char delaytime=15;
    static char speedmode=0;
    static char movx=1;
    static char LCDSHIFTX_EVERY_N=1;
    static char __attribute__((unused)) LCDSHIFTY_EVERY_N=1;
    char *nick=GLOBAL(nickname);


    lcdClear();
    setExtFont(GLOBAL(nickfont));
    nicky=1;
    nickwidth=DoString(nickx,nicky,nick);
    if(nickwidth<50)nickoff=30;
    nickheight=getFontHeight();
    nicky=(RESY-getFontHeight())/2;
    // Adjust speed depending on rendered image size
    if(nickwidth>RESX && nickheight>14) { movx=2;}
    if(nickwidth>RESX && nickheight>40) { movx=3;}

    lcdClear();
    char stepmode=0;

    while (1) {
        ++iter;
        lcdDisplay();
        lcdClear();
        lcdFill(GLOBAL(nickbg));
        setTextColor(GLOBAL(nickbg),GLOBAL(nickfg));
        // Old shift code. Can't handle longer Nicks...
        // if(iter%LCDSHIFT_EVERY_N==0) lcdShift(1,-2,1);
        // if(iter%LCDSHIFT_EVERY_N==0) { nickx=(nickx+1)%100-nickwidth; nicky=(nicky+1)%50;}
        if(iter%LCDSHIFTX_EVERY_N==0) { nickx-=movx;
        if(nickx<=(-1*nickwidth-nickoff))nickx=0; }
        DoString(nickx,nicky,nick);
        DoString(nickx+nickwidth+nickoff,nicky,nick);
        if(nickwidth<RESX) DoString(nickx+2*(nickwidth+nickoff),nicky,nick);
	char key=stepmode?getInputWait():getInputRaw();
	stepmode=0;
	switch(key) {
        // Buttons: Right change speed, Up hold scrolling
	case BTN_ENTER:
	  return;
	case BTN_RIGHT:
	  getInputWaitRelease();
          speedmode=(speedmode+1)%6;
          delaytime=15;
          // speeds: normal, slow, sloooow, double, tripple...
          switch(speedmode) {
            case 0:
              movx=1; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
            case 1:
              movx=1; LCDSHIFTX_EVERY_N=2; LCDSHIFTY_EVERY_N=2; break;
            case 2:
              movx=1; LCDSHIFTX_EVERY_N=3; LCDSHIFTY_EVERY_N=4; break;
            case 4:
              movx=2; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
            case 5:
              movx=3; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
          }
	  break;
	case BTN_UP:
	  stepmode=1;
	  getInputWaitRelease();
	  break;
	case BTN_LEFT:
          return;
	case BTN_DOWN:
	  return;
	}
        delayms_queue_plus(delaytime,0);
    }
    return;
}
