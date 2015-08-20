#include <stdio.h>
#include <string.h>
#include <ccccmaze/modules/navigate/initialize.h>
#include <ccccmaze/constants.h>
/**
 * setup globals
 */
void initialize(application_t *app){
	//todo: memset(maze,NULL,256*256)
	memset((void *) &app->screen_text, 0, sizeof(app->screen_text));
	memset((void *) &app->current_cell, 0, sizeof(app->current_cell));
	//todo: load from cm(namespace).cfg
	app->maze_position_x[0] = '0';
	app->maze_position_x[1] = '0';
	app->maze_position_x[2] = 0;
	app->maze_position_y[0] = '0';
	app->maze_position_y[1] = '0';
	app->maze_position_y[2] = 0;
	app->link_up[0] = '-';
	app->link_up[1] = '-';
	app->link_up[2] = '-';
	app->link_up[3] = '-';
	//app->link_up[4] = 0;
	app->link_down[0] = '0';
	app->link_down[1] = '0';
	app->link_down[2] = '0';
	app->link_down[3] = '1';
	//app->link_down[4] = 0;
	app->link_left[0] = '-';
	app->link_left[1] = '-';
	app->link_left[2] = '-';
	app->link_left[3] = '-';
	//app->link_left[4] = 0;
	app->link_right[0] = '0';
	app->link_right[1] = '1';
	app->link_right[2] = '0';
	app->link_right[3] = '0';
	//link_right[4] = 0;
	app->state = MAZE_STATE_MAZE;
}
