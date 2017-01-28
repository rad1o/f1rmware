#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/cm3/systick.h>

void init_speed(){
};

static uint8_t tickon=0;

void tick_speed(){
	static int ctr=0;
	if (tickon)
		if (++ctr%15==1)
			TOGGLE(RAD1O_LED4);
};

//# MENU speed
void speed_menu(){
	getInputWaitRelease();
	SETUPgout(RAD1O_LED4);
	tickon=1;
	lcdClear();
	lcdPrintln("Speed.");
	lcdPrintln("up:   204");
	lcdPrintln("down: 102");
	lcdPrintln("left:  12");
	lcdPrintln("right: 16");
	lcdDisplay();
	ON(RAD1O_LED1);

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
				OFF(RAD1O_LED4);
				return;
		};
		lcdSetCrsr(0,6*8);
		lcdPrint("Speed:");
		lcdPrint(IntToStr(_cpu_speed,4,F_LONG));
		lcdNl();
		lcdPrint("STK_CALIB:");
		lcdPrint(IntToStr(STK_CALIB,8,F_HEX));
		lcdNl();
		lcdDisplay();

		CGU_FREQ_MON = (0x0D << 24) | (1 << 23) | (256 << 0);
		while(CGU_FREQ_MON & (1 << 23));

		lcdPrint("FREQ_MON:");
		lcdPrint(IntToStr(CGU_FREQ_MON,8,F_HEX));
		lcdNl();

		uint32_t freq = ((CGU_FREQ_MON >> 9) & 0x3fFF) * 12e6 / 256.;
		lcdPrint("Measured:");
		lcdPrint(IntToStr(freq/1e6,4,F_LONG));
		lcdDisplay();
	};
};
