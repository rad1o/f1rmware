#include <r0ketlib/keyin.h>

// separation of code....change this?
#include <ccccmaze/modules/types/application.h>
#include <ccccmaze/modules/navigate/initialize.h>
#include <ccccmaze/modules/navigate/room.h>
#include <ccccmaze/modules/navigate/maze.h>
//# MENU level-0
void level_0_menu(){

	getInputWaitRelease();
	application_t app;
	app.state=1;
	initialize(&app);

	while(1){
		mazeShow(&app);
		mazeEvent(&app,getInput());
		getInputWaitRelease();
	};
};
