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
	const char* cx = IntToStr(maze_position_x,2,F_HEX);
    maze_filename[2] = cx[0];
    maze_filename[3] = cx[1];
    // IntToStr uses a static buffer
	const char* cy = IntToStr(maze_position_y,2,F_HEX);
    maze_filename[4] = cy[0];
    maze_filename[5] = cy[1];
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
		for ( int cpos = 0; cpos < MAX_ROOM_SIZE; cpos++){
			maze_text[cpos] = current_room[cpos];
			if ( last == 0xFF
				&& current_room[cpos] == 0xFF
				){
				maze_text[cpos] = 0;
				break;
				//todo: make image buffer
			}
			if ( current_room[cpos] == 0xFF ) {
				maze_text[cpos] = 0;
			}
			last = maze_text[cpos];
		}
		// default room actions
		for ( int room = 0; room < 8; room++){
			room_actions[room] = &roomLeave;
		}
		// if defined, map room actions -> see actions.h
	} else {
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