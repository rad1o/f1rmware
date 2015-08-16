// 30x20?
#define MAX_ROOM_SIZE 1024
#define RESTXTX 19
#define RESTXTY 16

char current_room[MAX_ROOM_SIZE + 1];
char maze_text[MAX_ROOM_SIZE + 1];
char maze_position_x[3];
char maze_position_y[3];
char link_up[4];
char link_down[4];
char link_left[4];
char link_right[4];
//void (*room_actions[8]);

#define MAZE_STATE_MAZE 0
#define MAZE_STATE_ROOM 1

int state;

void initialize(){
	//todo: memset(maze,NULL,256*256)
	maze_text[0] = 0;

	//todo: load from cm(namespace).cfg
	maze_position_x[0] = '0';
	maze_position_x[1] = '0';
	maze_position_x[2] = 0;
	maze_position_y[0] = '0';
	maze_position_y[1] = '0';
	maze_position_y[2] = 0;
	link_up[0] = '-';
	link_up[1] = '-';
	link_up[2] = '-';
	link_up[3] = '-';
	//link_up[4] = 0;
	link_down[0] = '0';
	link_down[1] = '0';
	link_down[2] = '0';
	link_down[3] = '1';
	//link_down[4] = 0;
	link_left[0] = '-';
	link_left[1] = '-';
	link_left[2] = '-';
	link_left[3] = '-';
	//link_left[4] = 0;
	link_right[0] = '0';
	link_right[1] = '1';
	link_right[2] = '0';
	link_right[3] = '0';
	//link_right[4] = 0;
	state = MAZE_STATE_MAZE;
}