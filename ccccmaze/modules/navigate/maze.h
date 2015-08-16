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
	if ( link_up[0] == '-' ) {
		lcdPrint("-------------");
    } else {
    	lcdPrint("      ^      ");
    }

	lcdPrint(maze_position_x);
	lcdPrint(" ");
	lcdPrint(maze_position_y);
	lcdPrintln("");

	//  render text of file padded
	int pos = 0;
	char line_buffer[RESTXTX];
	char padded_line_buffer[RESTXTX];
	char line_format[5];
	line_format[0] = ' ';
	line_format[1] = '%';
	line_format[2] = 's';
	line_format[3] = ' ';
	line_format[4] = 0;

	int col = 0;
	int row = 0;
	bool eot = false;
	for ( int i = 0; i < sizeof(line_buffer); i++ ){line_buffer[i] = ' ';}
	for ( int pos = 0; pos < MAX_ROOM_SIZE; pos++ ){
		if (link_left[0]=='-'){
			line_format[0]='|';
		} else {
			line_format[0]='<';
		}
		if (link_right[0]=='-'){
			line_format[3]='|';
		} else {
			line_format[3]='>';
		}

		if ( maze_text[pos] == 0
			|| row >= RESTXTY - 2
			){
			break;
		}
		if ( maze_text[pos] == '\n'
			|| col >= RESTXTX - 3
			){
			if ( maze_text[pos] != '\n' ){
				line_buffer[col] = maze_text[pos];
			}
			col = 0;
			row++;
			snprintf(padded_line_buffer,sizeof(padded_line_buffer),line_format,line_buffer);
			padded_line_buffer[RESTXTX - 1] = line_format[3];
			lcdPrintln(padded_line_buffer);
			for ( int i = 0; i < sizeof(line_buffer); i++ ){line_buffer[i] = ' ';}
			continue;
		}
		line_buffer[col] = maze_text[pos];
		col++;
	}
	snprintf(padded_line_buffer,sizeof(padded_line_buffer),line_format,line_buffer);
	lcdPrintln(padded_line_buffer);
	for ( row = row; row < RESTXTY - 2;row++){
		lcdPrintln("");
	}

	if ( link_down[0] == '-' ) {
		lcdPrint("------------------");
    } else {
    	lcdPrint("     \\/      ");
    }
	// display position x/y
	lcdDisplay();
}