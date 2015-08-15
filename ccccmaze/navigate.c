#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/fs_util.h>
#include "utils.h"

// separation of code....change this?


#include "modules/navigate/initialize.h"
#include "modules/navigate/room.h"
#include "modules/navigate/maze.h"

//# MENU navigate
void navigate_menu(){
	lcdClear();

	getInputWaitRelease();
	
	initialize();

	while(1){
		mazeShow();
		mazeEvent(getInput());
		getInputWaitRelease();
	};
};