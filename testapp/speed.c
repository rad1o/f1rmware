#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>

void init_speed(){
};

static uint8_t tickon=0;

void tick_speed(){
	static int ctr=0;
	if (tickon)
		if (++ctr%15==1)
			TOGGLE(LED4);
};

//# MENU speed
void speed_menu(){
	getInputWaitRelease();
	SETUPgout(LED4);
	tickon=1;
	lcdClear();
	lcdPrintln("Speed.");
	lcdPrintln("up:   204");
	lcdPrintln("down: 102");
	lcdPrintln("left:  12");
	lcdPrintln("right: 16");
	lcdDisplay();
	ON(LED1);

	while(1){
		switch(getInput()){
			case BTN_UP:
				cpu_clock_set(204);
				break;
			case BTN_DOWN:
				cpu_clock_set(102);
				break;
			case BTN_LEFT:
				cpu_clock_set(12);
				break;
			case BTN_RIGHT:
				cpu_clock_set(16);
				break;
			case BTN_ENTER:
				tickon=0;
				OFF(LED4);
				return;
		};
		lcdSetCrsr(0,6*8);
		lcdPrint("Speed:");
		lcdPrint(IntToStr(_cpu_speed,4,F_LONG));
		lcdNl();
		lcdDisplay();
	};
};
