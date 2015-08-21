#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>

//# MENU usage
void usage_menu(){
	uint32_t read_count;

	lcdClear();
	getInputWaitRelease();

	/*
	 * todo: load text from file
	 */
	getInputWaitRelease();
	lcdClear();
	lcdPrintln("cccc 2015 maze");
	lcdPrintln("");
	lcdPrintln("use cursor to");
	lcdPrintln("move up and down");
	lcdPrintln("switch off to end");
	lcdPrintln("");
	lcdPrintln("fork on GH and");
	lcdPrintln("contribute!");
	lcdPrintln("");
	lcdPrintln("https://github.com/");
	lcdPrintln("rad1o/f1rmware");
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
