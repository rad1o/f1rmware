void mazeEvent(int key_event);
void mazeInfo();
void mazeLoad();
void mazeMove(int key_event);

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
		maze_position_y--;
		break;
	case BTN_DOWN:
		maze_position_y++;
		break;
	case BTN_LEFT:
		maze_position_x--;
		break;
	case BTN_RIGHT:
		maze_position_x++;
		break;
	};
	// check boundaries -> info
	bool corrected = false;
	if ( maze_position_x < 0 ){
		maze_position_x = 0;
		corrected = true;
	}
	if ( maze_position_y < 0 ){
		maze_position_y = 0;
		corrected = true;
	}
	if ( maze_position_x > MAX_MAZE_X ){
		maze_position_x = MAX_MAZE_X;
		corrected = true;
	}
	if ( maze_position_x > MAX_MAZE_Y ){
		maze_position_x = MAX_MAZE_Y;
		corrected = true;
	}
	if ( corrected ){
		mazeInfo("its a camp, it has boundaries");
	} else {
		// store ccccmaze.cfg -> todo:fn
		char cfg_position[2];
		cfg_position[0] = (char)maze_position_x;
		cfg_position[1] = (char)maze_position_y;
		writeFile("cm_position.cfg",cfg_position,sizeof(cfg_position));
	}
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
	// 8< mazeLoad

	//  split lcd-image, show
	// maximum characters as blanks - FFx00
	lcdPrint("             ");
	lcdPrint(IntToStr(maze_position_x,2,F_HEX));
	lcdPrint(" ");
	lcdPrint(IntToStr(maze_position_y,2,F_HEX));
	lcdPrintln("");
	//  render text of file
	lcdPrintln(maze_text);
	// display position x/y
	lcdDisplay();
}