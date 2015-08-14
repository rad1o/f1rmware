// more!
#define MAX_ROOM_SIZE 256
#define MAX_MAZE_X 256
#define MAX_MAZE_Y 256

static char maze[MAX_MAZE_X][MAX_MAZE_Y];
char current_room[MAX_ROOM_SIZE + 1];
char maze_text[MAX_ROOM_SIZE + 1];
int maze_position_x;
int maze_position_y;
void (*room_actions[8]);

#define MAZE_STATE_MAZE 0
#define MAZE_STATE_ROOM 1

int state;

void initialize(){
	//todo: memset(maze,NULL,256*256)
	maze_text[0] = 0;
	maze_position_x = 0;
	maze_position_y = 0;
	state = MAZE_STATE_MAZE;
}