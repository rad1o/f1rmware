#include <sysinit.h>

#include "basic/basic.h"

#include "lcd/render.h"
#include "lcd/display.h"
#include "lcd/allfonts.h"
#include "usetable.h"

#define FIXSIZE 25
#define mul(a,b) ((((long long)a)*(b))>>FIXSIZE)
#define fixpt(a) ((long)(((a)*(1<<FIXSIZE))))
#define integer(a) (((a)+(1<<(FIXSIZE-1)))>>FIXSIZE)

#define ZOOM_RATIO 0.90
#define ITERATION_MAX 150

void mandelInit();
void mandelMove();
void mandelUpdate();

void ram(void) {
    int key;
    mandelInit();
    while (1) {
        lcdDisplay();
        mandelMove();
        mandelUpdate();
        // Exit on enter+left
        key=getInputRaw();
        if(key== BTN_ENTER + BTN_LEFT)
            return;
    }
    return;
}

struct mb {
    long rmin, rmax, imin, imax;
    bool dirty, dup, ddown, dleft, dright, clickmark;
    int presscount, presslimitzin, presslimitzout, zoomlevel, maxzoomin, maxzoomout;
} mandel;

void mandelInit() {
    //mandel.rmin = -2.2*0.9;
    //mandel.rmax = 1.0*0.9;
    //mandel.imin = -2.0*0.9;
    //mandel.imax = 2.0*0.9;
    mandel.rmin = fixpt(-2);
    mandel.rmax = fixpt(1);
    mandel.imin = fixpt(-2);
    mandel.imax = fixpt(2);
    mandel.presscount = 0;
    mandel.presslimitzin = 15;    
    mandel.presslimitzout = 5;
    mandel.zoomlevel = 0;
    mandel.maxzoomin = 42;
    mandel.maxzoomout = -12;

    mandel.dirty = true;
    mandel.dup = false;
    mandel.ddown = false;
    mandel.dleft = false;
    mandel.dright = false;
    mandel.clickmark = false;
}

void mandelMove() {
    //long delta_r = (mandel.rmax - mandel.rmin)/10;
    //long delta_i = (mandel.imax - mandel.imin)/10;
 
	long rs =(mandel.rmax-mandel.rmin)/RESY;
	long is =(mandel.imax-mandel.imin)/RESX;

	char key = getInputWaitTimeout(10);

	if (key == BTN_LEFT) {
		mandel.imax -=is;
		mandel.imin -=is;
		mandel.dleft = true;
	} else if (key == BTN_RIGHT) {
		mandel.imax += is;
		mandel.imin += is;
		mandel.dright = true;
	} else if (key == BTN_DOWN) {
		mandel.rmax += rs;
		mandel.rmin += rs;
		mandel.ddown = true;
	} else if (key == BTN_UP) {
		mandel.rmax -= rs;
		mandel.rmin -= rs;
		mandel.dup = true;
	} else if (key == BTN_ENTER) {
		if (mandel.presscount < mandel.presslimitzin) {
			mandel.presscount = mandel.presscount + 1;
		}
	} else if (key == BTN_NONE) {
		//delayms_queue(15);
		if(mandel.presscount > 0 ) {
			mandel.presscount = mandel.presscount - 1;
			mandel.clickmark = true;
		}
		if (mandel.presscount == 0 ) {
			mandel.clickmark = false;
		}
	}
	if (mandel.presscount > mandel.presslimitzout && mandel.clickmark && key == BTN_ENTER && mandel.zoomlevel >= mandel.maxzoomout) {
		mandel.imin = mandel.imin - (mandel.imax-mandel.imin)/8;
		mandel.imax = mandel.imax + (mandel.imax-mandel.imin)/8;
		mandel.rmin = mandel.rmin -(mandel.rmax-mandel.rmin)/8;
		mandel.rmax = mandel.rmax +(mandel.rmax-mandel.rmin)/8;
        mandel.dirty = true;
        delayms(10);
        mandel.zoomlevel = mandel.zoomlevel - 1 ;
	 }
       else if (mandel.presscount == mandel.presslimitzin && key == BTN_ENTER && mandel.zoomlevel <= mandel.maxzoomin ) {
        mandel.imin = mandel.imin + (mandel.imax-mandel.imin)/8;
        mandel.imax = mandel.imax - (mandel.imax-mandel.imin)/8;
        mandel.rmin = mandel.rmin +(mandel.rmax-mandel.rmin)/8;
        mandel.rmax = mandel.rmax -(mandel.rmax-mandel.rmin)/8;
        mandel.dirty = true;
        delayms(10);
        mandel.zoomlevel = mandel.zoomlevel + 1 ;
     } 
}

void mandelPixel(int x, int y) {
    long r0,i0,rn, p,q;
    long rs,is;
    int iteration;

    rs=(mandel.rmax-mandel.rmin)/RESY;
    is=(mandel.imax-mandel.imin)/RESX;
    //p=fixpt(mandel.rmin+y*rs);
    //q=fixpt(mandel.imin+x*is);
    p=mandel.rmin+y*rs;
	q=mandel.imin+x*is;
            
	rn=0;
    r0=0;
    i0=0;
    iteration=0;
    while ((mul(rn,rn)+mul(i0,i0))<fixpt(4) && ++iteration<ITERATION_MAX)  {
        rn=mul((r0+i0),(r0-i0)) +p;
        i0=mul(fixpt(2),mul(r0,i0)) +q;
        r0=rn;
    }
    if (iteration==ITERATION_MAX) iteration=1;
    bool pixel = ( iteration>1);
    lcdSetPixel(x, y, pixel);
}

void mandelUpdate() {
    int xmin,xmax,ymin,ymax;
    if (mandel.dirty) {
        xmin = 0;
        xmax = RESX;
        ymin = 0;
        ymax = RESY;
        mandel.dirty = false;
    } else if (mandel.dleft) {
        lcdShift(1,0,false);
        xmin = 0;
        xmax = 1;
        ymin = 0;
        ymax = RESY;
        mandel.dleft = false;
    } else if (mandel.dright) {
        lcdShift(-1,0,false);
        xmin = RESX-1;
        xmax = RESX;
        ymin = 0;
        ymax = RESY;
        mandel.dright = false;
    } else if (mandel.dup) {
        lcdShift(0,-1,true);
        xmin=0;
        xmax=RESX;
        ymin=0;
        ymax=1;
        mandel.dup = false;
    } else if (mandel.ddown) {
        lcdShift(0,1,true);
        xmin=0;
        xmax=RESX;
        ymin=RESY-1;
        ymax=RESY;
        mandel.ddown = false;
    } else {
        return;
    }

    for (int x = xmin; x<xmax; x++) {
        for (int y = ymin; y<ymax; y++) {
            mandelPixel(x,y);
        }
    }
}

