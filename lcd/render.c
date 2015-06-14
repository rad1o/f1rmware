#include <string.h>

#include <render.h>
//#include <decoder.h>
#include <fonts.h>
// #include "basic/basic.h"
#include "fonts/smallfonts.h"

// #include "filesystem/ff.h"
#include "render.h"

/* Global Variables */
const struct FONT_DEF * font = NULL;

struct EXTFONT efont;

#ifdef NOFILE
static FIL file; /* current font file */
#endif

/* Exported Functions */

void setIntFont(const struct FONT_DEF * newfont){
    memcpy(&efont.def,newfont,sizeof(struct FONT_DEF));
    efont.type=FONT_INTERNAL;
    font=&efont.def;
}

void setExtFont(const char *fname){
    if(strlen(fname)>8+4)
        return;
    strcpy(efont.name,fname);
//    memcpy(efont.name+strlen(fname),".f0n",5);

    efont.type=FONT_EXTERNAL;
    font=NULL;
}

int getFontHeight(void){
    if(font)
      return font->u8Height;
    return 8; // XXX: Should be done right.
}

#ifdef NOFILE
static uint8_t read_byte (void)
{
  UINT    readbytes;
  uint8_t byte;
  f_read(&file, &byte, sizeof(uint8_t), &readbytes);
  return byte;
}

int _getFontData(int type, int offset){
    UINT readbytes;
    UINT res;
    static uint16_t extras;
    static uint16_t character;
//    static const void * ptr;

    if(efont.type == FONT_EXTERNAL){

    if (type == START_FONT){
        efont.def.u8Width = read_byte ();
        efont.def.u8Height = read_byte ();
        efont.def.u8FirstChar = read_byte ();
        efont.def.u8LastChar = read_byte ();
        res = f_read(&file, &extras, sizeof(uint16_t), &readbytes);
        return 0;
    };
    if (type == SEEK_EXTRAS){
        f_lseek(&file,6);
        return 0;
    };
    if(type == GET_EXTRAS){
        uint16_t word;
        res = f_read(&file, &word, sizeof(uint16_t), &readbytes);
        return word;
    };
    if (type == SEEK_WIDTH){
        f_lseek(&file,6+(extras*sizeof(uint16_t)));
        return 0;
    };
    if(type == GET_WIDTH || type == GET_DATA){
        return read_byte ();
    };
    if(type == SEEK_DATA){
        character=offset;
        f_lseek(&file,6+
                (extras*sizeof(uint16_t))+
                ((extras+font->u8LastChar-font->u8FirstChar)*sizeof(uint8_t))+
                (offset*sizeof(uint8_t))
                );
        return 0;
    };
    if(type == PEEK_DATA){
        uint8_t width;
        width = read_byte ();
        f_lseek(&file,6+
                (extras*sizeof(uint16_t))+
                ((extras+font->u8LastChar-font->u8FirstChar)*sizeof(uint8_t))+
                (character*sizeof(uint8_t))
                );
        return width;
    };
#ifdef NOTYET
    }else{ // efont.type==FONT_INTERNAL

        if (type == START_FONT){
            memcpy(&efont.def,font,sizeof(struct FONT_DEF));
            return 0;
        };
        if (type == SEEK_EXTRAS){
            ptr=efont.def.charExtra;
            return 0;
        };
        if(type == GET_EXTRAS){
            uint16_t word;
            word=*(uint16_t*)ptr;
            ptr=((uint16_t*)ptr)+1;
            return word;
        };
        if (type == SEEK_WIDTH){
            ptr=efont.def.charInfo;
            return 0;
        };
        if(type == GET_WIDTH || type == GET_DATA){
            uint8_t width;
            width=*(uint8_t*)ptr;
            ptr=((uint8_t*)ptr)+1;
            return width;
        };
        if(type == SEEK_DATA){
            if(offset==REPEAT_LAST_CHARACTER)
                offset=character;
            else
                character=offset;

            ptr=efont.def.au8FontTable;
            return 0;
        };
#endif
    };

    /* NOTREACHED */
    return 0;
}

#endif

static int _getIndex(int c){
#define ERRCHR (font->u8FirstChar+1)
    /* Does this font provide this character? */
    if(c<font->u8FirstChar)
        c=ERRCHR;
    if(c>font->u8LastChar && efont.type!=FONT_EXTERNAL && font->charExtra == NULL)
        c=ERRCHR;

    if(c>font->u8LastChar && (efont.type==FONT_EXTERNAL || font->charExtra != NULL)){
        if(efont.type==FONT_EXTERNAL){
#ifdef NOFILE
            _getFontData(SEEK_EXTRAS,0);
            int cc=0;
            int cache;
            while( (cache=_getFontData(GET_EXTRAS,0)) < c)
                cc++;
            if( cache > c)
                c=ERRCHR;
            else
                c=font->u8LastChar+cc+1;
#endif
        }else{
            int cc=0;
            while( font->charExtra[cc] < c)
                cc++;
            if(font->charExtra[cc] > c)
                c=ERRCHR;
            else
                c=font->u8LastChar+cc+1;
        };
    };
    c-=font->u8FirstChar;
    return c;
}

uint8_t charBuf[MAXCHR];

int DoChar(int sx, int sy, int c){

//    font=NULL;
    if(font==NULL){
        if(efont.type==FONT_INTERNAL){
            font=&efont.def;
#ifdef NOFILE
        }else if (efont.type==FONT_EXTERNAL){
            UINT res;
            res=f_open(&file, efont.name, FA_OPEN_EXISTING|FA_READ);
            if(res){
                efont.type=0;
                font=&Font_7x8;
            }else{
                _getFontData(START_FONT,0);
                font=&efont.def;
            };
#endif
        }else{
            font=&Font_7x8;
        };
    };

	/* how many bytes is it high? */
	char height=(font->u8Height);

	const uint8_t * data;
    int width,preblank=0,postblank=0; 
    do { /* Get Character data */
        /* Get intex into character list */
        c=_getIndex(c);

        /* starting offset into character source data */
        int toff=0;

        if(font->u8Width==0){
            if(efont.type == FONT_EXTERNAL){
#ifdef NOFILE
                _getFontData(SEEK_WIDTH,0);
                for(int y=0;y<c;y++)
                    toff+=_getFontData(GET_WIDTH,0);
                width=_getFontData(GET_WIDTH,0);

                _getFontData(SEEK_DATA,toff);
                UINT res;
                UINT readbytes;
                UINT size = width * height;
                if(size > MAXCHR) size = MAXCHR;
                res = f_read(&file, charBuf, size, &readbytes);
                if(res != FR_OK || readbytes<width*height)
                    return sx;
                data=charBuf;
#endif
            }else{
                for(int y=0;y<c;y++)
                    toff+=font->charInfo[y].widthBits;
                width=font->charInfo[c].widthBits;

                toff*=height;
                data=&font->au8FontTable[toff];
            };
            postblank=1;
#ifdef NOFILE
        }else if(font->u8Width==1){ // NEW CODE
            if(efont.type == FONT_EXTERNAL){
                _getFontData(SEEK_WIDTH,0);
                for(int y=0;y<c;y++)
                    toff+=_getFontData(GET_WIDTH,0);
                width=_getFontData(GET_WIDTH,0);
                _getFontData(SEEK_DATA,toff);
                UINT res;
                UINT readbytes;
                uint8_t testbyte;
                testbyte = read_byte ();
                if(testbyte>>4 ==15){
                    preblank = read_byte ();
                    postblank = read_byte ();
                    width-=3;
                    width/=height;
                    UINT size = width * height;
                    if(size > MAXCHR) size = MAXCHR;
                    res = f_read(&file, charBuf, size, &readbytes);
                    if(res != FR_OK || readbytes<width*height)
                        return sx;
                    data=charBuf;
                }else{
                    _getFontData(SEEK_DATA,toff);
                    data=pk_decode(NULL,&width); // Hackety-hack
                };
            }else{
                // Find offset and length for our character
                for(int y=0;y<c;y++)
                    toff+=font->charInfo[y].widthBits;
                width=font->charInfo[c].widthBits;
                if(font->au8FontTable[toff]>>4 == 15){ // It's a raw character!
                    preblank = font->au8FontTable[toff+1];
                    postblank= font->au8FontTable[toff+2];
                    data=&font->au8FontTable[toff+3];
                    width=(width-3/height);
                }else{
                    data=pk_decode(&font->au8FontTable[toff],&width);
                }
            };

#endif
        }else{
            toff=(c)*font->u8Width*1; // XXX: actually height/8 instead of 1
            width=font->u8Width;
            data=&font->au8FontTable[toff];
        };

    }while(0);

#define xy_(x,y) ((y)*RESX+(x))
#define gPx(x,y) (data[x]&(1<<y))

	int x=0;

    /* Our display height is non-integer. Adjust for that. */
//    sy+=RESY%8;

    /* Fonts may not be byte-aligned, shift up so the top matches */
//    sy-=hoff;

    sx+=preblank;

	/* per line */
	for(int y=0;y<=height;y++){
        if(sy+y>=RESY)
            continue;

        /* Optional: empty space to the left */
		for(int b=1;b<=preblank;b++){
            if(sx>=RESX)
                continue;
			lcdBuffer[xy_(sx-b,sy+y)]=0xff;
		};
        /* Render character */
		for(x=0;x<width;x++){
            if(sx+x>=RESX)
                continue;
			if (gPx(x,y)){
				lcdBuffer[xy_(sx+x,sy+y)]=0b00000011;
			}else{
				lcdBuffer[xy_(sx+x,sy+y)]=0xff;
			};
		};
        /* Optional: empty space to the right */
		for(int m=0;m<postblank;m++){
            if(sx+x+m>=RESX)
                continue;
			lcdBuffer[xy_(sx+x+m,sy+y)]=0xff;
		};
	};
	return sx+(width+postblank);
}

#define UTF8
// decode 2 and 3-byte utf-8 strings.
#define UT2(a)   ( ((a[0]&31)<<6)  + (a[1]&63) )
#define UT3(a)   ( ((a[0]&15)<<12) + ((a[1]&63)<<6) + (a[2]&63) )

int DoString(int sx, int sy, const char *s){
	const char *c;
	int uc;
	for(c=s;*c!=0;c++){
#ifdef UTF8
		/* will b0rk on non-utf8 */
		if((*c&(128+64+32))==(128+64) && c[1]!=0){
			uc=UT2(c); c+=1;
			sx=DoChar(sx,sy,uc);
		}else if( (*c&(128+64+32+16))==(128+64+32) && c[1]!=0 && c[2] !=0){
			uc=UT3(c); c+=2;
			sx=DoChar(sx,sy,uc);
		}else
#endif
		sx=DoChar(sx,sy,*c);
	};
	return sx;
}
