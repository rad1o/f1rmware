void mazeEvent(int key_event);
void mazeInfo();
void mazeLoad();
void mazeMove(int key_event);
void mazeShow();

void mazeEvent(int key_event){
	// evaluate current state
	// decide how maze is navigated
	if ( MAZE_STATE_MAZE == state ){
		return mazeMove(key_event);
	}
	if ( MAZE_STATE_ROOM == state ){
		return roomEvent(key_event);
	}
}
void mazeMove(int key_event){
	// user cursor +/-
	switch(key_event){
	case BTN_UP:
		maze_position_x[0] = link_up[0];
		maze_position_x[1] = link_up[1];
		maze_position_y[0] = link_up[2];
		maze_position_y[1] = link_up[3];
		break;
	case BTN_DOWN:
		maze_position_x[0] = link_down[0];
		maze_position_x[1] = link_down[1];
		maze_position_y[0] = link_down[2];
		maze_position_y[1] = link_down[3];
		break;
	case BTN_LEFT:
		maze_position_x[0] = link_left[0];
		maze_position_x[1] = link_left[1];
		maze_position_y[0] = link_left[2];
		maze_position_y[1] = link_left[3];
		break;
	case BTN_RIGHT:
		maze_position_x[0] = link_right[0];
		maze_position_x[1] = link_right[1];
		maze_position_y[0] = link_right[2];
		maze_position_y[1] = link_right[3];
		break;
	};
	/**
	// store ccccmaze.cfg -> todo:fn
	char cfg_position[4];
	cfg_position[0] = maze_position_x[0];
	cfg_position[1] = maze_position_x[1];
	cfg_position[2] = maze_position_y[0];
	cfg_position[3] = maze_position_y[1];
	writeFile("cm_position.cfg",cfg_position,sizeof(cfg_position));
	*/
}
void mazeInfo(const char * text){
	lcdPrint("info:");
	lcdPrint(" ");
	lcdPrint(text);
	lcdPrintln("");
}
void mazeShow(){
	lcdClear();
	// render image? top half of screen
	// load maze_file if exists (cache?)
	roomLoad();

	//  split lcd-image, show
	// maximum characters as blanks - FFx00
	lcdPrint("             ");
	lcdPrint(maze_position_x);
	lcdPrint(" ");
	lcdPrint(maze_position_y);
	lcdPrintln("");
	//  render text of file
	for ( int i = 0; i < sizeof(maze_text); i++ ){
		if ( maze_text[i] == 0 ){
			break;
		}
		if ( maze_text[i] == '\n' ){
			lcdPrintln("");
			continue;
		}
		char str[2];
		str[1] = 0;
		str[0] = maze_text[i];
		lcdPrint(str);
	}
	// display position x/y
	lcdDisplay();
}