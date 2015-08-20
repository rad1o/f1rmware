#ifndef __application_h_
#define __application_h_

#include "../../constants.h"

typedef struct {
	char maze_position_x[3];
	char maze_position_y[3];
	char link_up[5];
	char link_down[5];
	char link_left[5];
	char link_right[5];
	char namespace_name[3];
	int state;
	char current_cell[MAX_ROOM_SIZE + 1];
	char screen_text[MAX_ROOM_SIZE + 1];
} application_t;
#endif
