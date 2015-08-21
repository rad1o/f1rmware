#ifndef __debug_h_
#define __debug_h_
//# ifdef DEBUG
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>

//# endif
/**
  * utilities to make c-code readable
  */

/**
 * print some text
 */
void debug(const char* text){
//# ifdef DEBUG
	lcdPrintln("");
	lcdPrint("debug:");
	lcdPrintln(text);
	lcdPrintln("");
	lcdDisplay();
//# endif
}
#endif
