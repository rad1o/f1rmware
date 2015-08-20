#ifndef __application_h_
#define __application_h_

#include "../../constants.h"

typedef struct {
	char current_cell[MAX_ROOM_SIZE + 1];
	char screen_text[MAX_ROOM_SIZE + 1];
	char maze_position_x[3];
	char maze_position_y[3];
	char link_up[4];
	char link_down[4];
	char link_left[4];
	char link_right[4];
	char namespace_name[2];
	int state;
} application_t;
#endif
