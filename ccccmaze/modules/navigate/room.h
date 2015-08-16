void roomEvent(int key_event);
void roomLoad();
void roomLeave();

void roomEvent(int key_event){
// 1 2 3 4  should be some room path
	roomLeave();
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
void roomLoad(){

	// ascii - save bytes, add name
	char maze_filename[12];
    maze_filename[0] = 'c';
    maze_filename[1] = 'm';
    maze_filename[2] = maze_position_x[0];
    maze_filename[3] = maze_position_x[1];
    maze_filename[4] = maze_position_y[0];
    maze_filename[5] = maze_position_y[1];
    maze_filename[6] = '.';
    maze_filename[7] = 'c';
    maze_filename[8] = 'f';
    maze_filename[9] = 'g';
    maze_filename[10] = '\0';
	if ( 0 < readTextFile(maze_filename,current_room,MAX_ROOM_SIZE) ){
		// text format to allow easy customize
		// FF FF starts image in lcd-format (130x?)
		// out  scope?
		// maze_text = new char(MAX_ROOM_SIZE)
		// strcpy(maze_text,current_room)
		char last = 0;
		/** link rooms first line
		 * eg: "--------00000200" allows only right/left movement
		 * use gen to create a pattern
		 * evaluated when the button is pressed
		 */

		#define LINK_SIZE 16
		link_up[0] = current_room[0];
		link_up[1] = current_room[1];
		link_up[2] = current_room[2];
		link_up[3] = current_room[3];
		link_down[0] = current_room[4];
		link_down[1] = current_room[5];
		link_down[2] = current_room[6];
		link_down[3] = current_room[7];
		link_left[0] = current_room[8];
		link_left[1] = current_room[9];
		link_left[2] = current_room[10];
		link_left[3] = current_room[11];
		link_right[0] = current_room[12];
		link_right[1] = current_room[13];
		link_right[2] = current_room[14];
		link_right[3] = current_room[15];
		int offset = LINK_SIZE + 1;
		for ( int cpos = offset
			; cpos < MAX_ROOM_SIZE
			; cpos++
			){
			maze_text[cpos - offset] = current_room[cpos];
			if ( last == 0xFF
				&& current_room[cpos] == 0xFF
				){
				maze_text[cpos - offset] = 0;
				break;
				//todo: make image buffer
			}
			if ( current_room[cpos] == 0xFF ) {
				maze_text[cpos - offset] = 0;
			}
			last = maze_text[cpos - offset];
		}
		// default room actions
		//for ( int room = 0; room < 8; room++){
		//	room_actions[room] = &roomLeave;
		//}
		// if defined, map room actions -> see actions.h
	} else {
		link_up[0] = '0';
		link_up[1] = '0';
		link_up[2] = '0';
		link_up[3] = '0';
		link_down[0] = '0';
		link_down[1] = '0';
		link_down[2] = '0';
		link_down[3] = '0';
		link_left[0] = '0';
		link_left[1] = '0';
		link_left[2] = '0';
		link_left[3] = '0';
		link_right[0] = '0';
		link_right[1] = '0';
		link_right[2] = '0';
		link_right[3] = '0';

		maze_text[0] = 'n';
		maze_text[1] = 'o';
		maze_text[2] = 't';
		maze_text[3] = 'h';
		maze_text[4] = 'i';
		maze_text[5] = 'n';
		maze_text[6] = 'g';
		maze_text[7] = 'i';
		maze_text[8] = 's';
		maze_text[9] = 'h';
		maze_text[10] = 'e';
		maze_text[11] = 'r';
		maze_text[12] = 'e';
		maze_text[13] = 0;
	}
}
void roomLeave(){
	state = MAZE_STATE_MAZE;
}