#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/execute.h>

#include <libopencm3/lpc43xx/adc.h>

//# MENU execute
void execute_menu(){
	getInputWaitRelease();
	executeSelect("C1D");
};
