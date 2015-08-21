#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <ccccmaze/modules/navigate/display.h>
void displayBottomBar(bool maze_edge){
	if ( maze_edge ) {
		lcdPrint(" -----------------");
    } else {
    	lcdPrint(" \\/\\/\\/\\/\\/\\/\\/\\/");
    }
	// display position x/y
	lcdDisplay();
}
void displayRow(const char * text){
	lcdPrint(text);
	lcdPrintln("");
}
void displayToast(const char * text){
	lcdPrintln("");
	lcdPrint("info:");
	lcdPrint(" ");
	lcdPrint(text);
	lcdPrintln("");
}

void displayTopBar(bool maze_edge, const char * info_a, const char * info_b){
	lcdClear();

	if ( maze_edge ) {
		lcdPrint(" ------------");
	} else {
		lcdPrint(" ^^^^^^^^^^^^");
	}
	lcdPrint(info_a);
	lcdPrint(" ");
	lcdPrint(info_b);
	lcdPrintln("");

}
