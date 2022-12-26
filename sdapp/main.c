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
	cpuClockInit(); /* CPU Clock is now 104 MHz */
	systickInit();

//	cpu_clock_set(204);

	SETUPgout(EN_VDD);
	SETUPgout(EN_1V8);
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

	while(1){
//		if(getInput()){
			handleMenu(&menu_main);
			getInputWaitRelease();
//		};
//		fancyNickname();
	};
	return 0;
}
