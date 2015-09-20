#include <string.h>

#include <r0ketlib/display.h>
#include <r0ketlib/print.h>

#include <fatfs/ff.h>
#include <r0ketlib/select.h>
#include <r0ketlib/execute.h>
#include <r0ketlib/config.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fs_util.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/stringin.h>
#include <r0ketlib/colorin.h>

#include <string.h>

/***************************************************************************/

//# MENU key keyInputRaw
void keyTestRaw(void) {
    lcdClear();
    lcdPrintln("Press enter to increse count");
    lcdPrintln("with getInputRaw()");
    lcdPrintln("exit with left");
    lcdNl();
    lcdSetCrsr(0, 50);
    lcdPrint("Up");
    lcdSetCrsr(0, 60);
    lcdPrint("Enter");
    lcdDisplay();

    int countEnter;
    int countUp;

    while( getInputRaw() != BTN_LEFT) {
	if (getInputRaw() == BTN_ENTER){
	    lcdSetCrsr(50, 60);
	    countEnter ++;
	    lcdPrintInt(countEnter);
	    lcdDisplay();
	}

	if (getInputRaw() == BTN_UP){
	    lcdSetCrsr(50, 50);
	    countUp ++;
	    lcdPrintInt(countUp);
	    lcdDisplay();
	}

    }

}

//# MENU key keyInputRaw_edge
//Counts on rising edge of keyInputRaw
void keyTestRawEdge(void) {
    lcdClear();
    lcdPrintln("Press enter to increse count");
    lcdPrintln("with getInputRaw()");
    lcdPrintln("exit with left");
    lcdNl();
    lcdSetCrsr(0, 50);
    lcdPrint("Up");
    lcdSetCrsr(0, 60);
    lcdPrint("Enter");
    lcdDisplay();

    int countEnter;
    int countUp;

    uint8_t lastInput = BTN_NONE;
    uint8_t currentInput = BTN_NONE;
    
    while( getInputRaw() != BTN_LEFT) {
	currentInput = getInputRaw();	
	if (currentInput == BTN_ENTER && lastInput != BTN_ENTER){
	    lcdSetCrsr(50, 60);
	    countEnter ++;
	    lcdPrintInt(countEnter);
	    lcdDisplay();
	}

	if (currentInput == BTN_UP && lastInput != BTN_UP){
	    lcdSetCrsr(50, 50);
	    countUp ++;
	    lcdPrintInt(countUp);
	    lcdDisplay();
	}
	lastInput = currentInput;
    }

}
