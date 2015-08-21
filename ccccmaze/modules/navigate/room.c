#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/config.h>

#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <r0ketlib/fs_util.h>

#include <stdio.h>

#include <ccccmaze/constants.h>
#include <ccccmaze/modules/navigate/display.h>
#include <ccccmaze/modules/navigate/room.h>

void roomEvent(application_t *app,int key_event){
// 1 2 3 4  should be some room path
	roomLeave(app);
	/**
	 * error: called object is not a function or function pointer
	switch(key_event){
	case BTN_UP:
		(*room_actions[0])();
		break;
	case BTN_DOWN:
		(*room_actions[1])();
		break;
	case BTN_LEFT:
		(*room_actions[2])();
		break;
	case BTN_RIGHT:
		(*room_actions[3])();
		break;
	case BTN_ENTER:
		(*room_actions[4])();
		break;
	case BTN_NONE:
		(*room_actions[5])();
		break;
	};
	*/
}
void roomLoad(application_t *app){

	// ascii - save bytes, add name
	char maze_filename[12];
	snprintf(maze_filename,sizeof(maze_filename),"%s%s%s.csm",app->namespace_name,app->maze_position_x,app->maze_position_y);
	if ( 0 < readTextFile(maze_filename,app->current_cell,MAX_ROOM_SIZE) ){
		// text format to allow easy customize
		// FF FF starts image in lcd-format (130x?)
		// out  scope?
		// screen_text = new char(MAX_ROOM_SIZE)
		// strcpy(screen_text,current_cell)
		char last = 0;
		/** link rooms first line
		 * eg: "--------00000200" allows only right/left movement
		 * use gen to create a pattern
		 * evaluated when the button is pressed
		 */

		#define LINK_SIZE 16
		snprintf(app->link_up,sizeof(app->link_up),"%s",app->current_cell + 0);
		snprintf(app->link_down,sizeof(app->link_down),"%s",app->current_cell + 4);
		snprintf(app->link_left,sizeof(app->link_left),"%s",app->current_cell + 8);
		snprintf(app->link_right,sizeof(app->link_right),"%s",app->current_cell + 12);
		int offset = LINK_SIZE + 1;
		for ( int cpos = offset
			; cpos < MAX_ROOM_SIZE
			; cpos++
			){
			app->screen_text[cpos - offset] = app->current_cell[cpos];
			if ( last == 0xFF
				&& app->current_cell[cpos] == 0xFF
				){
				app->screen_text[cpos - offset] = 0;
				break;
				//todo: make image buffer
			}
			if ( app->current_cell[cpos] == 0xFF ) {
				app->screen_text[cpos - offset] = 0;
			}
			last = app->screen_text[cpos - offset];
		}
		// default room actions
		//for ( int room = 0; room < 8; room++){
		//	room_actions[room] = &roomLeave;
		//}
		// if defined, map room actions -> see actions.h
	} else {
		snprintf(app->link_up,sizeof(app->link_up),"%s","----");
		snprintf(app->link_down,sizeof(app->link_down),"%s","----");
		snprintf(app->link_left,sizeof(app->link_left),"%s","----");
		snprintf(app->link_right,sizeof(app->link_right),"%s","----");
		snprintf(app->screen_text,sizeof(app->screen_text),"%s\n%s",maze_filename,"no maze found\nplease reinstall");
	}
}
void roomLeave(application_t *app){
	app->state = MAZE_STATE_MAZE;
}
