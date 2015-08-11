/*
 * This file is part of rad1o
 *
 */

#include <unistd.h>
#include <string.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/config.h>
#include <r0ketlib/print.h>
#include <r0ketlib/stringin.h>
#include <r0ketlib/night.h>

#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <r0ketlib/fs_util.h>

#include "main.gen"

void infoscreen(){
    lcdClear();
    lcdPrintln("-------------------");
    lcdPrintln("-RAD1O BADGE SETUP-");
    lcdPrintln("---- FW REV 01 ----");
    lcdNl();
    lcdPrintln("To enter BOOT Menu");
    lcdPrintln("hold Joystick");
    lcdPrintln("while switching");
    lcdPrintln("badge on");
    lcdNl();
    lcdPrintln("LEFT: BOOT SELECT");
    lcdPrintln("DOWN: DFU mode");
    lcdNl();
    lcdPrintln("UP:   MSC");
    lcdPrintln("  (to copy files");
    lcdPrintln("     to badge)");
    lcdDisplay();
    getInputWait();
    getInputWaitRelease();
};

#define EVERY(x,y) if((ctr+y)%(x/SYSTICKSPEED)==0)
void night_tick(void){
    static int ctr;
    ctr++;

    EVERY(1024,0){
//        if(!adcMutex){
//            VoltageCheck();
        LightCheck();
    };

    static char night=0;
    static char posleds = 0;
    EVERY(128,2){
        if(night!=isNight()){
            night=isNight();
            if(night){
                ON(LCD_BL_EN);
//                push_queue(queue_unsetinvert);
            }else{
                OFF(LCD_BL_EN);
//                push_queue(queue_setinvert);
           };
        };
    };

#if 0
    EVERY(50,0){
        if(GLOBAL(chargeled)){
            char iodir= (GPIO_GPIO1DIR & (1 << (11) ))?1:0;
            if(GetChrgStat()) {
                if (iodir == gpioDirection_Input){
                    IOCON_PIO1_11 = 0x0;
                    gpioSetDir(RB_LED3, gpioDirection_Output);
                    gpioSetValue (RB_LED3, 1);
                    LightCheck();
                }
            } else {
                if (iodir != gpioDirection_Input){
                    gpioSetValue (RB_LED3, 0);
                    gpioSetDir(RB_LED3, gpioDirection_Input);
                    IOCON_PIO1_11 = 0x41;
                    LightCheck();
                }
            }
        };

        if(GetVoltage()<3600){
            IOCON_PIO1_11 = 0x0;
            gpioSetDir(RB_LED3, gpioDirection_Output);
            if( (ctr/(50/SYSTICKSPEED))%10 == 1 )
                gpioSetValue (RB_LED3, 1);
            else
                gpioSetValue (RB_LED3, 0);
        };
    };
#endif

    return;
}

void sys_tick_handler(void){
	incTimer();
    night_tick();
	generated_tick();
};

void fancyNickname(void);

int main(void) {
	cpu_clock_init(); /* CPU Clock is now 104 MHz */
	ssp_clock_init();
	systickInit();

//	cpu_clock_set(204);

	SETUPgout(EN_VDD);
	SETUPgout(MIXER_EN);
	SETUPgout(MIC_AMP_DIS);

	SETUPgout(LED1);
	SETUPgout(LED2);
	SETUPgout(LED3);
	SETUPgout(LED4);

	inputInit();
	lcdInit();
	fsInit(); 
	lcdFill(0xff);
	readConfig();

	generated_init();

    init_nick();

    readConfig();
    if(getInputRaw()==BTN_RIGHT){
        GLOBAL(develmode)=1;
        applyConfig();
    };

    // XXX: TODO!
    // randomInit();

    if(GLOBAL(version)==0){ // no config (yet?)
        infoscreen();
        input("Nickname?", GLOBAL(nickname), 32, 127, MAXNICK-1);
        getInputWaitRelease();
        writeFile("nick.cfg",GLOBAL(nickname),strlen(GLOBAL(nickname)));
        saveConfig();
    };

    menuflags|=MENU_TIMEOUT;

	while(1){
		if(getInput()){
			handleMenu(&menu_main);
			getInputWaitRelease();
		};
		fancyNickname();
	};
	return 0;
}
