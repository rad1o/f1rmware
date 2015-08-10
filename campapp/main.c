/*
 * This file is part of rad1o
 *
 */

#include <unistd.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/config.h>

#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <r0ketlib/fs_util.h>

#include "main.gen"

void sys_tick_handler(void){
	incTimer();
	generated_tick();
};

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
    if(GLOBAL(version)==0){ // no config (yet?)
        lcdPrintln("-------------------");
        lcdPrintln("-RAD1O BADGE SETUP-");
        lcdPrintln("-------------------");
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
