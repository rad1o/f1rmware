#include <r0ketlib/config.h>
#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>

#include "usetable.h"



#define BITSET_X (RESX+2)
#define BITSET_Y (RESY+2)
#define BITSET_SIZE (BITSET_X*BITSET_Y)

#define BITSETCHUNKSIZE 32

#define one ((uint32_t)1)

struct bitset {
  uint16_t size;
  uint32_t bits[BITSET_SIZE/BITSETCHUNKSIZE+1];
};


typedef uint8_t uchar;

int pattern=0;
#define PATTERNCOUNT 3


uchar stepmode=0;
uchar randdensity=0;
static unsigned long iter=0;

struct bitset _life;
#define life (&_life)

static void draw_area();
static void calc_area();
static void random_area(struct bitset *area, uchar x0, uchar y0, uchar x1, uchar y1,uchar value);
static void reset_area();

void ram(void) {
    getInputWaitRelease();
    reset_area();
    random_area(life,1,1,RESX,RESY,40);

    static int nickx=2,nicky=10;
    signed char movy=1;
    static int nickwidth,nickheight;
    static int nickoff=10;
    static char delaytime=10;
    static char speedmode=0;
    static char LCDSHIFTX_EVERY_N=2;
    static char LCDSHIFTY_EVERY_N=2;
    static char ITER_EVERY_N=1;

    lcdClear();
    setExtFont(GLOBAL(nickfont));
    
    nickwidth=DoString(nickx,nicky,GLOBAL(nickname));
    if(nickwidth<50)nickoff=30;
    nickheight=getFontHeight();

    char stepmode=0;
    while (1) {
        draw_area(); // xor life pattern over display content
        lcdDisplay();
        lcdClear();
        // draw_area(); // xor life pattern again to restore original display content
        // Old shift code. Can't handle longer Nicks...
        // if(iter%LCDSHIFT_EVERY_N==0) lcdShift(1,-2,1);
        // if(iter%LCDSHIFT_EVERY_N==0) { nickx=(nickx+1)%100-nickwidth; nicky=(nicky+1)%50;}
        if(iter%LCDSHIFTX_EVERY_N==0) { nickx--; 
        if(nickx<(-1*nickwidth-nickoff))nickx=0; }
        if(iter%LCDSHIFTY_EVERY_N==0) { nicky+=movy;
        if(nicky<1 || nicky>RESY-nickheight) movy*=-1; }
        DoString(nickx,nicky,GLOBAL(nickname));
        DoString(nickx+nickwidth+nickoff,nicky,GLOBAL(nickname));
        if(nickwidth<RESX) DoString(nickx+2*(nickwidth+nickoff),nicky,GLOBAL(nickname));
	char key=stepmode?getInputWait():getInputRaw();
	stepmode=0;
	switch(key) {
	case BTN_ENTER:
	  return;
	case BTN_RIGHT:
	  getInputWaitRelease();
          speedmode=(speedmode+1)%7;
          delaytime=15;
          switch(speedmode) {
            case 0:
              ITER_EVERY_N=1; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
            case 1:
              ITER_EVERY_N=1; LCDSHIFTX_EVERY_N=2; LCDSHIFTY_EVERY_N=2; break;
            case 2:
              ITER_EVERY_N=1; LCDSHIFTX_EVERY_N=4; LCDSHIFTY_EVERY_N=4; break;
            case 3:
              ITER_EVERY_N=2; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
            case 4:
              ITER_EVERY_N=4; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
            case 5:
              ITER_EVERY_N=8; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
            case 6:
              delaytime=5; ITER_EVERY_N=8; LCDSHIFTX_EVERY_N=1; LCDSHIFTY_EVERY_N=1; break;
          }
	  break; 
	case BTN_DOWN:
	  stepmode=1;
	  getInputWaitRelease();
	  break;
	case BTN_LEFT:
	  pattern=(pattern+1)%PATTERNCOUNT;
	case BTN_UP:
	  stepmode=1;
	  reset_area();
	  getInputWaitRelease();
	  break;
	}
        delayms_queue_plus(delaytime,0);
#ifdef SIMULATOR
  fprintf(stderr,"Iteration %d - x %d, y %d \n",iter,nickx,nicky);
#endif
        if(iter%ITER_EVERY_N==0) calc_area(); else ++iter;
    }
    return;
}

static inline void bitset_set(struct bitset *bs,uint16_t index, uint8_t value) {
  uint16_t base=index/BITSETCHUNKSIZE;
  uint16_t offset=index%BITSETCHUNKSIZE;
  if(value) {
    bs->bits[base]|=(one<<offset);
  } else {
    bs->bits[base]&=~(one<<offset);
  }
}

static inline void bitset_xor(struct bitset *bs,uint16_t index, uint8_t value) {
  uint16_t base=index/BITSETCHUNKSIZE;
  uint16_t offset=index%BITSETCHUNKSIZE;
  if(value) {
    bs->bits[base]^=(one<<offset);
  }
}

static inline uint8_t bitset_get(struct bitset *bs,uint16_t index) {
  uint16_t base=index/BITSETCHUNKSIZE;
  uint16_t offset=index%BITSETCHUNKSIZE;
  return (bs->bits[base]&(one<<offset))==(one<<offset);;
}

static inline uint16_t bitset_offset2(uint8_t x, uint8_t y) {
  return ((uint16_t)x)+((uint16_t)y)*BITSET_X;
}

static inline void bitset_set2(struct bitset *bs, uint8_t x, uint8_t y, uint8_t value) {
  bitset_set(bs,bitset_offset2(x,y),value);
}

static inline void bitset_xor2(struct bitset *bs, uint8_t x, uint8_t y, uint8_t value) {
  bitset_xor(bs,bitset_offset2(x,y),value);
}

static inline uint8_t bitset_get2(struct bitset *bs,uint8_t x,uint8_t y) {
  return bitset_get(bs,bitset_offset2(x,y));
}

static void fill_area(struct bitset *area, uchar x0, uchar y0, uchar x1, uchar y1,uchar value) {
  for(uchar x=x0; x<=x1; ++x) {
    for(uchar y=y0; y<=y1; ++y) {
      bitset_set2(area,x,y,value);
    }
  }
} 

static void draw_area() {
  for(uchar x=0; x<RESX; ++x) {
    for(uchar y=0; y<RESY; ++y) {
      // lcdSetPixel(x,y,lcdGetPixel(x,y)^bitset_get2(life,x+1,y+1));
      if((lcdGetPixel(x,y)==0) ^ bitset_get2(life,x+1,y+1)) {
        lcdSetPixel(x,y,0xff);
      } else {
        lcdSetPixel(x,y,0);
      }
    }
  }
}

static void copy_col(uint8_t columnindex, uint8_t *columnbuffer) {
  for(uchar y=0; y<=RESY+1; ++y) {
    columnbuffer[y]=bitset_get2(life,columnindex,y);
  }
}

static void calc_area() {
  ++iter;
  // sweeping mutation point
  static uint8_t xiter=0;
  static uint8_t yiter=0;
  xiter=(xiter+1)%RESX;
  if(xiter==0) yiter=(yiter+1)%RESY;
  bitset_set2(life,xiter+1,yiter+1,1);

  // remember just two columns
  // these don´t have to be static, so if the stack is big enoguh put them there and save another 200 bytes?
  static uint8_t _a[RESY+2],*left=_a;
  static  uint8_t _b[RESY+2],*middle=_b;
  copy_col(0,left);
  copy_col(1,middle);
  for(uchar x=1; x<=RESX; ++x) {
    for(uchar y=1; y<=RESY; ++y) {
      uchar sum=bitset_get2(life,x+1,y-1)+bitset_get2(life,x+1,y)+bitset_get2(life,x+1,y+1)+
	left[y-1]+left[y]+left[y+1]+middle[y-1]+middle[y+1];
      bitset_set2(life,x,y,sum==3||(sum==2&&bitset_get2(life,x,y)));
    }
    // temp-less swap of buffer pointers
    left+=(uint32_t)middle;
    middle=left-(uint32_t)middle;
    left=left-(uint32_t)middle;
    copy_col(x+1,middle);
  }
}

static void reset_area() {
  fill_area(life,0,0,RESX+1,RESY+1,0);
  
  switch(pattern) {
  case 0: // R pentomino
    bitset_set2(life,41,40,1);
    bitset_set2(life,42,40,1);
    bitset_set2(life,41,41,1);
    bitset_set2(life,40,41,1);
    bitset_set2(life,41,42,1);
    break;
  case 1: // block in the center, continuous generators at the edges
    for(int i=0; i<RESX/2+3; ++i) bitset_set2(life,i,0,1);
    //    for(int i=0; i<RESY; ++i) bitset_set2(life,0,i,1);
    //    for(int i=0; i<RESY/2-20; ++i) bitset_set2(life,RESX+1,RESY-i,1);
    //    for(int i=0; i<RESX/2; ++i) bitset_set2(life,RESX-i,RESY+1,1);
    bitset_set2(life,40,40,1);
    bitset_set2(life,41,40,1);
    bitset_set2(life,42,40,1);
    bitset_set2(life,42,41,1);
    bitset_set2(life,42,42,1);
    bitset_set2(life,40,41,1);
    bitset_set2(life,40,42,1);
    break;
#if 0
  case 2: // _|^|_
    bitset_set2(life,40,40,1);
    bitset_set2(life,41,40,1);
    bitset_set2(life,42,40,1);
    bitset_set2(life,42,41,1);
    bitset_set2(life,42,42,1);
    bitset_set2(life,40,41,1);
    bitset_set2(life,40,42,1);
    break;
#endif
  }
}

static void random_area(struct bitset *area, uchar x0, uchar y0, uchar x1, uchar y1,uchar value) {
  for(uchar x=x0; x<=x1; ++x) {
    for(uchar y=y0; y<=y1; ++y) {
      bitset_set2(area,x,y,(getRandom()>>24)<value);
    }
  }
}
