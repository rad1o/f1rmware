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
    lcdPrintln("getInputRaw()");
    lcdPrintln("Press left to exit");
    lcdNl();
    lcdSetCrsr(0, 50);
    lcdPrint("Up");
    lcdSetCrsr(0, 60);
    lcdPrint("Enter");
    lcdSetCrsr(0, 70);
    lcdPrint("Down");
    lcdDisplay();

    int countEnter;
    int countUp;
    int countDown;

    while( getInputRaw() != BTN_LEFT) {
	if (getInputRaw() == BTN_UP){
	    lcdSetCrsr(50, 50);
	    countUp ++;
	    lcdPrintInt(countUp);
	    lcdDisplay();
	}
	if (getInputRaw() == BTN_ENTER){
	    lcdSetCrsr(50, 60);
	    countEnter ++;
	    lcdPrintInt(countEnter);
	    lcdDisplay();
	}

	if (getInputRaw() == BTN_DOWN){
	    lcdSetCrsr(50, 70);
	    countDown ++;
	    lcdPrintInt(countDown);
	    lcdDisplay();
	}

    }

}

//# MENU key keyInputRaw_edge
//Counts on rising edge of keyInputRaw
void keyTestRawEdge(void) {
    lcdClear();
    lcdPrintln("getInputRaw()");
    lcdPrintln("with edge detection");
    lcdPrintln("Press left to exit");
    lcdNl();
    lcdSetCrsr(0, 50);
    lcdPrint("Up");
    lcdSetCrsr(0, 60);
    lcdPrint("Enter");
    lcdSetCrsr(0, 70);
    lcdPrint("Down");
    lcdDisplay();

    int countEnter;
    int countUp;
    int countDown;

    uint8_t lastInput = BTN_NONE;
    uint8_t currentInput = BTN_NONE;
    
    while( getInputRaw() != BTN_LEFT) {
	currentInput = getInputRaw();	
	if (currentInput == BTN_UP && lastInput != BTN_UP){
	    lcdSetCrsr(50, 50);
	    countUp ++;
	    lcdPrintInt(countUp);
	    lcdDisplay();
	}
	if (currentInput == BTN_ENTER && lastInput != BTN_ENTER){
	    lcdSetCrsr(50, 60);
	    countEnter ++;
	    lcdPrintInt(countEnter);
	    lcdDisplay();
	}

	if (currentInput == BTN_DOWN && lastInput != BTN_DOWN){
	    lcdSetCrsr(50, 50);
	    countDown ++;
	    lcdPrintInt(countDown);
	    lcdDisplay();
	}
	lastInput = currentInput;
    }

}

//# MENU key keyInput
//Counts on getInputChange and getInput
void keyTestDebounce(void) {
    lcdClear();
    lcdPrintln("Press enter to increse count");
    lcdPrintln("with getInput");
    lcdPrintln("Press left to exit");
    lcdNl();
    lcdSetCrsr(0, 50);
    lcdPrint("Up");
    lcdSetCrsr(0, 60);
    lcdPrint("Enter");
    lcdSetCrsr(0, 70);
    lcdPrint("Down");
    lcdDisplay();

    int countEnter;
    int countUp;
    int countDown;

    uint8_t changeInput = BTN_NONE;
    uint8_t currentInput = BTN_NONE;
    
    while( getInputRaw() != BTN_LEFT) {
	currentInput = getInput();
	changeInput = getInputChange();	
	if (currentInput == BTN_UP && changeInput == BTN_UP){
	    lcdSetCrsr(50, 50);
	    countUp ++;
	    lcdPrintInt(countUp);
	    lcdDisplay();
	}
	if (currentInput == BTN_ENTER && changeInput == BTN_ENTER){
	    lcdSetCrsr(50, 60);
	    countEnter ++;
	    lcdPrintInt(countEnter);
	    lcdDisplay();
	}
	if (currentInput == BTN_DOWN && changeInput == BTN_DOWN){
	    lcdSetCrsr(50, 70);
	    countDown ++;
	    lcdPrintInt(countDown);
	    lcdDisplay();
	}

    }

}

//# MENU key WaitRepeat
void keyTestWaitRepeat(void) {
    lcdClear();
    lcdPrintln("Press enter to increse count");
    lcdPrintln("with getInputWaitRepeat()");
    lcdPrintln("Press left to exit");
    lcdNl();
    lcdSetCrsr(0, 50);
    lcdPrint("Up");
    lcdSetCrsr(0, 60);
    lcdPrint("Enter");
    lcdSetCrsr(0, 70);
    lcdPrint("Down");
    lcdDisplay();

    int countEnter = 0;
    int countUp = 0;
    int countDown = 0;

    while( getInputRaw() != BTN_LEFT) {
	if (getInputWaitRepeat() == BTN_UP){
	    lcdSetCrsr(50, 50);
	    countUp ++;
	    lcdPrintInt(countUp);
	    lcdDisplay();
	}

	if (getInputWaitRepeat() == BTN_ENTER){
	    lcdSetCrsr(50, 60);
	    countEnter ++;
	    lcdPrintInt(countEnter);
	    lcdDisplay();
	}

	if (getInputWaitRepeat() == BTN_DOWN){
	    lcdSetCrsr(50, 70);
	    countDown ++;
	    lcdPrintInt(countDown);
	    lcdDisplay();
	}

    }

}

