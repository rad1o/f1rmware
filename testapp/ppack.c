/*
 * Copyright 2015 team rad1o
 *
 */

#include <unistd.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <r0ketlib/fs_util.h>
#include <rad1olib/pins.h>

#include <portapack_hackrf/portapack.h>
#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <libopencm3/lpc43xx/dac.h>


extern uint32_t sctr;
extern uint16_t *sram;

//# MENU Apack
void ppack_menu() {
	lcdClear();
	lcdPrintln("PPack port");
	lcdPrintln("");
	lcdPrintln("up=enable");
	lcdDisplay();
	dac_init(false);

	cpu_clock_set(204); // WARP SPEED! :-)
	hackrf_clock_init();
	rf_path_pin_setup();
	/* Configure external clock in */
	scu_pinmux(SCU_PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);

	sgpio_configure_pin_functions();

	ON(EN_VDD);
	ON(EN_1V8);
	OFF(MIC_AMP_DIS);
	uint16_t * samples;

	while(1){
		switch(getInputRaw()){
			case BTN_UP:
			    // ON(MIXER_EN); // hackrf does this
			    cpu_clock_set(204); // WARP SPEED! :-)
			    si5351_init();
			    portapack_init();
			    getInputWaitRelease();

			    break;
			case BTN_DOWN:
			    lcdPrintln("audio");
			    for(samples=(uint16_t*)0x20000000;samples<sram;samples++){
				dac_set(samples[0]);
				delayNop(765);
			    };

			    break;
			case BTN_LEFT:
			    lcdPrintln("reset");
			    sram=(uint16_t*)0x20000000;
			    break;
			case BTN_RIGHT:
				break;
			case BTN_ENTER:
				return;
		};
		TOGGLE(LED2);
		delayms(40);
		lcdPrint(IntToStr((uintptr_t)sram,8,F_HEX));
		lcdPrint(" ");
		lcdPrintln(IntToStr(sctr,7,F_LONG));
		lcdDisplay();
	};
};
