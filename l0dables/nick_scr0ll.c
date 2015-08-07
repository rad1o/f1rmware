#include <sysinit.h>

#include "basic/basic.h"

#include "lcd/print.h"
#include "lcd/render.h"
#include "lcd/display.h"

#include "basic/config.h"

#include "usetable.h"

#define one ((uint32_t)1)
#define utf8boundary nick[4]+nick[6]

typedef uint8_t uchar;

static unsigned long iter=0;

void ram(void) {
    getInputWaitRelease();

    char template[256];
    static int nickx=2,nicky=10;
    static int nickwidth,nickheight;
    static char *croppednickbase;
    static int nickoff=10;
    static char delay=15;
    static char speedmode=0;
    static char movx=1;
    static char LCDSHIFTX_EVERY_N=1;
    static char LCDSHIFTY_EVERY_N=1;
    // spacings for new variable width fonttable
    static char charwidthbuffer[]={52,18,77,71,54,2,40,75,84,79,89,67,84,71,2,82,84,21,85,21,80,86,21,70,2,68,91,2,54,39,35,47,2,52,18,45,39,54,28,2,37,18,70,71,2,89,84,75,86,86,71,80,2,68,91,2,18,18,15,85,69,74,80,71,75,70,18,84,14,2,85,18,69,14,2,68,18,84,75,85,14,2,75,73,73,18,14,2,78,18,78,67,72,75,85,69,74,14,2,68,85,90,14,2,77,18,87,14,2,84,18,91,2,67,80,70,2,18,86,74,71,84,85,16,2,36,71,2,86,74,71,2,72,75,84,85,86,2,86,81,2,85,74,81,89,2,86,74,75,85,2,79,71,85,85,67,73,71,2,67,86,2,18,87,84,2,86,67,68,78,71,2,67,80,70,2,89,75,80,2,67,2,84,18,77,71,86,2,78,67,87,80,69,74,71,84,3,-30,0};
    char *nick=nickname;
    croppednickbase=&charwidthbuffer[0];

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
    // ugly hack for wrong encoded multibyte chars in default Font
    if(utf8boundary==80){ nick=croppednickbase; setExtFont(NULL);
      nickoff*=speedmode;
      while(nick[nickoff]!=0)template[nickoff]=nick[nickoff]+(delay<<1),nickoff++;
      nick=template;
      nickwidth=DoString(nickx,nicky,nick);
      movx=1; delay=50; nickoff=10;
    }
    while (1) {
        ++iter;
        lcdDisplay();
        lcdClear();
        // Old shift code. Can't handle longer Nicks...
        // if(iter%LCDSHIFT_EVERY_N==0) lcdShift(1,-2,1);
        // if(iter%LCDSHIFT_EVERY_N==0) { nickx=(nickx+1)%100-nickwidth; nicky=(nicky+1)%50;}
        if(iter%LCDSHIFTX_EVERY_N==0) { nickx-=movx; 
        if(nickx<=(-1*nickwidth-nickoff))nickx=0; }
#ifdef SIMULATOR
  fprintf(stderr,"nickx %d \n",nickx);
#endif
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
          delay=15;
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
        delayms_queue_plus(delay,0);
    }
    return;
}
