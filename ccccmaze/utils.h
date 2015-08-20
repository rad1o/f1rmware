//# ifdef DEBUG
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
//# endif
/**
  * utilities to make c-code readable
  */

/**
 * print some text
 */
void debug(const char* text){
//# ifdef DEBUG
	lcdPrintln(text);
	lcdPrintln("");
	lcdDisplay();
//# endif
}
