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

void ram(void);

void sys_tick_handler(void){
	incTimer();
	generated_tick();
};

int main(void) {
	cpuClockInit(); /* CPU Clock is now 104 MHz */
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

	while(1) {
        handleMenu(&menu_main);
        getInputWaitRelease();
	}
	return 0;
}
