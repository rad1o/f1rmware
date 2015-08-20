#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/config.h>

#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <r0ketlib/fs_util.h>

#include <stdio.h>

#include <ccccmaze/constants.h>
#include <ccccmaze/modules/navigate/display.h>
#include <ccccmaze/modules/navigate/maze.h>
#include <ccccmaze/modules/navigate/room.h>

void mazeEvent(application_t *app,int key_event){
	// evaluate current state
	// decide how maze is navigated
	if ( MAZE_STATE_MAZE == app->state ){
		mazeMove(app,key_event);
		return;
	}
	if ( MAZE_STATE_ROOM == app->state ){
		roomEvent(app,key_event);
		return;
	}
}
void mazeMove(application_t *app,int key_event){
	// user cursor +/-
	switch(key_event){
	case BTN_UP:
		app->maze_position_x[0] = app->link_up[0];
		app->maze_position_x[1] = app->link_up[1];
		app->maze_position_y[0] = app->link_up[2];
		app->maze_position_y[1] = app->link_up[3];
		break;
	case BTN_DOWN:
		app->maze_position_x[0] = app->link_down[0];
		app->maze_position_x[1] = app->link_down[1];
		app->maze_position_y[0] = app->link_down[2];
		app->maze_position_y[1] = app->link_down[3];
		break;
	case BTN_LEFT:
		app->maze_position_x[0] = app->link_left[0];
		app->maze_position_x[1] = app->link_left[1];
		app->maze_position_y[0] = app->link_left[2];
		app->maze_position_y[1] = app->link_left[3];
		break;
	case BTN_RIGHT:
		app->maze_position_x[0] = app->link_right[0];
		app->maze_position_x[1] = app->link_right[1];
		app->maze_position_y[0] = app->link_right[2];
		app->maze_position_y[1] = app->link_right[3];
		break;
	};
	/**
	// store ccccmaze.cfg -> todo:fn
	char cfg_position[4];
	cfg_position[0] = app->maze_position_x[0];
	cfg_position[1] = app->maze_position_x[1];
	cfg_position[2] = app->maze_position_y[0];
	cfg_position[3] = app->maze_position_y[1];
	writeFile("cm_position.cfg",cfg_position,sizeof(cfg_position));
	*/
}

void mazeShow(application_t *app){
	// render image? top half of screen
	// load maze_file if exists (cache?)
	roomLoad(app);

	//  split lcd-image, show
	// maximum characters as blanks - FFx00
	displayTopBar((app->link_up[0] == '-'), app->maze_position_x, app->maze_position_y );

	//  render text of file padded
	int pos = 0;
	char line_buffer[RESTXTX];
	char padded_line_buffer[RESTXTX];
	char line_format[5];
	line_format[0] = ' ';
	line_format[1] = '%';
	line_format[2] = 's';
	line_format[3] = ' ';
	line_format[4] = 0;

	int col = 0;
	int row = 0;
	for ( int i = 0; i < sizeof(line_buffer); i++ ){line_buffer[i] = ' ';}
	if (app->link_left[0]=='-'){
		line_format[0]='|';
	} else {
		line_format[0]='<';
	}
	if (app->link_right[0]=='-'){
		line_format[3]='|';
	} else {
		line_format[3]='>';
	}
	for ( int pos = 0; pos < MAX_ROOM_SIZE; pos++ ){

		if ( app->screen_text[pos] == 0
			|| row >= RESTXTY - 2
			){
			break;
		}
		if ( app->screen_text[pos] == '\n'
			|| col >= RESTXTX - 3
			){
			if ( app->screen_text[pos] != '\n' ){
				line_buffer[col] = app->screen_text[pos];
			}
			col = 0;
			row++;
			snprintf(
				  padded_line_buffer
				, sizeof(padded_line_buffer)
				, line_format
				, line_buffer
				);
			padded_line_buffer[RESTXTX - 1] = line_format[3];
			displayRow(padded_line_buffer);
			for ( int i = 0; i < sizeof(line_buffer); i++ ){
				line_buffer[i] = ' ';
			}
			pos++;
			continue;
		}
		line_buffer[col] = app->screen_text[pos];
		col++;
	}

	for ( int i = 1; i < RESTXTX - 1; i++ ){
		padded_line_buffer[i] = ' ';
	}
	for ( row = row; row < RESTXTY - 2; row++){
		displayRow(padded_line_buffer);
	}
	displayBottomBar(app->link_down[0] == '-');
}
