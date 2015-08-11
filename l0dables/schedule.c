#include <stdint.h>
#include <string.h>

#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/idle.h>
#include <r0ketlib/fs_util.h>
#include <r0ketlib/config.h>

#ifdef L0DABLE
#include "usetable.h"
#endif


#define one ((uint32_t)1)
#define DoInt(x,y,i) DoString(x,y,IntToStr(i,6,F_LONG));

typedef uint8_t uchar;

static unsigned long iter=0;

static int getint3fromf(FIL *file);

//
#ifdef L0DABLE
void ram(void) {
#else
//# MENU schedule
void fahrplan_menu() {
#endif

    getInputWaitRelease();

    // static char delay=15;
   
    char buf[20];

    int line;
    int startline=0;
    char *message=NULL;
    char filvers;
    char favers[5]; 
    unsigned char ob;
    int eventid;
    int evnext;
    int evprv;
    int evcur=5;


    lcdClear();
    /* setExtFont(GLOBAL(nickfont)); */
    setExtFont(NULL);

    /* nickheight=getFontHeight(); */

    lcdClear();

        FIL file;
        FRESULT res;
        UINT readbytes;
    
        res=f_open(&file, "FAHRPLAN.SCD", FA_OPEN_EXISTING|FA_READ|FA_WRITE);
        if(res) message="FOPEN ERROR";
        res=f_read(&file, &filvers, 1, &readbytes); 
        res+=f_read(&file, (char *)favers, 4, &readbytes); 
        favers[4]=0;


        lcdClear();
        DoString(0,0,"Fahrplan ");
        DoString(60,0,favers);
        DoString(0,16,"SW Rel. VR0.1 ");
        if(filvers!=2) {
        DoString(0,24,"Incompatible  ");
        DoString(0,32,"Binary. Get   ");
        DoString(0,40,"update from   ");
        DoString(0,48,"r0ket.de      ");
        DoString(0,56,"Filefmt.: ");
        DoInt(60,56,filvers);
        lcdDisplay();
	getInputWait();
	getInputWaitRelease();
        return;
        } 
        DoString(0,24,"Check for     ");
        DoString(0,32,"Updates on    ");
        DoString(0,40,"rad1o.de      ");
        DoString(0,48,"              ");
        /* DoInt(0,24,filvers); */
        lcdDisplay();
        delayms(100);
        DoString(0,56," PRESS BUTTON ");
        lcdDisplay();
	getInputWait();
	getInputWaitRelease();

        evcur=getint3fromf(&file);

    while (1) {
        ++iter;
        lcdDisplay();
        lcdClear();
        char title[15];
        char info[15];
        int evtext; 

#ifdef SIMULATOR
#endif
#ifndef SIMULATOR
        f_lseek(&file,evcur);

        eventid=getint3fromf(&file);
        evprv=getint3fromf(&file);
        evnext=getint3fromf(&file);

        res+=f_read(&file, (char *)title, 15, &readbytes); 
        res+=f_read(&file, (char *)info, 15, &readbytes); 
        evtext=f_tell(&file);

#endif
        /* for(line=0; line<4; line++) {
          DoString(0, 8*line+8,tdes[0][line+startline]);
        } */
#ifndef SIMULATOR
        title[14]=0;
        DoString(16,2,title);
        info[14]=0;
        DoString(16,10,info);

        int notend=1; int endc;
        for(line=0; line<13 && notend; line++) {
        f_lseek(&file,evtext+(startline+line)*17);
        res+=f_read(&file, (char *)buf, 18, &readbytes); 
        if(res) message="READ ERROR"; else message="READ: 0";
        notend=1;
        for(endc=0;endc<17;endc++) {
          if(buf[0]==0)notend=0;
        }
        buf[17]=0;
        DoString(5,8*line+22,buf);
        if(!buf[0])notend=0;
        } 

/* Debug:
        DoInt(0,32,eventid);
        DoInt(0,40,evprv);
        DoInt(50,40,evnext);
        if(message) DoString(0, 8*5+8,message);
*/ 
#endif
        lcdDisplay();

	// char key=stepmode?getInputWait():getInputRaw();
	//
	char key=getInputWait();
	switch(key) {
        // Buttons: Right change speed, Up hold scrolling
	case BTN_ENTER:
	  getInputWaitRelease();
          f_lseek(&file,5);
          ob=evcur%256;
          res+=f_write(&file, (char *)&ob, 1, &readbytes);
          ob=(evcur>>8) % 256;
          res+=f_write(&file, (char *)&ob, 1, &readbytes);
          ob=evcur>>16;
          res+=f_write(&file, (char *)&ob, 1, &readbytes);
	  return;
	case BTN_RIGHT:
	  getInputWaitRelease();
          if(evnext)evcur=evnext;
          startline=0;
          break;
	case BTN_UP:
	  getInputWaitRelease();
          if(startline)startline--;
	  break;
	case BTN_LEFT:
	  getInputWaitRelease();
          if(evprv)evcur=evprv;
          startline=0;
          break;
	case BTN_DOWN:
	  getInputWaitRelease();
          if(notend)startline++;
	  break;
	}
        // delayms_queue_plus(delay,0);
    }
    return;
}

static int getint3fromf(FIL *file) {
    unsigned char ob;
    UINT readbytes;
    int res;

    f_read(file, (char *)&ob, 1, &readbytes); 
    res=ob;
    f_read(file, (char *)&ob, 1, &readbytes); 
    res+=ob*256;
    f_read(file, (char *)&ob, 1, &readbytes); 
    res+=ob*256*256;

    return res;
}
