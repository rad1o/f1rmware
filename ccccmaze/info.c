#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>

//# MENU info
void info_menu(){
	uint32_t read_count;

	lcdClear();
	getInputWaitRelease();
	
	/*
	 * todo: load text from file
	 */
	getInputWaitRelease();
	lcdClear();
	lcdPrintln("text-mode");
	lcdPrintln(" up/down more text");
	lcdPrintln(" enter return");
	lcdPrintln("navigation mode");
	lcdPrintln(" joystick to select room");
	lcdPrintln(" enter to enter text-mode");
	lcdPrintln(" switch off to leave");
	lcdPrintln("info mode");
	lcdPrintln(" any key");
	lcdPrintln(" return quit");
	lcdPrintln("");
	lcdDisplay();
	while (1){
		switch(getInput()){
			case BTN_UP:
			case BTN_DOWN:
			case BTN_LEFT:
			case BTN_RIGHT:
			case BTN_ENTER:
				return;
				break;
		};	
	}
	return;
};