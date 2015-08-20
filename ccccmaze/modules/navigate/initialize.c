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
	snprintf(app->namespace_name,sizeof(app->namespace_name),"cm");
	snprintf(app->maze_position_x,sizeof(app->maze_position_x),"00");
	snprintf(app->maze_position_y,sizeof(app->maze_position_y),"00");
	snprintf(app->link_up,sizeof(app->link_up),"%s","----");
	snprintf(app->link_down,sizeof(app->link_down),"%s","0001");
	snprintf(app->link_left,sizeof(app->link_left),"%s","----");
	snprintf(app->link_right,sizeof(app->link_right),"%s","0100");
	app->state = MAZE_STATE_MAZE;
}
