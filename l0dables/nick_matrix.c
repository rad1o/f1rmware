#include <sysinit.h>

#include "basic/basic.h"
#include "basic/config.h"
#include "lcd/render.h"

#include "usetable.h"

#define SCREEN_WIDTH 13
#define SCREEN_HEIGHT 8

#define FONT_WIDTH 7
#define FONT_HEIGHT 8

#define MATRIX_LINES_LENGTH 15

#define MAX_LINE_LENGTH 6

#define TICKS_SHOW_NICKNAME 100
#define TICKS_HIDE_NICKNAME 50
#define NICKNAME_ACTION_SHOW 1
#define NICKNAME_ACTION_HIDE 2

void ram(void){
	lcdClear();

	// nickname helper variables
	int nickname_len = strlen(GLOBAL(nickname));
	int nickname_posx = (SCREEN_WIDTH - nickname_len) / 2;
	int nickname_posy = SCREEN_HEIGHT / 2;

	// state variables for show/hide effect
	int ticks_until_next_nickname_action = TICKS_SHOW_NICKNAME;
	int nickname_action = NICKNAME_ACTION_SHOW;

	struct matrix_line {
		int head_x, head_y;
		int cur_length;
		int length;
	} matrix_lines[MATRIX_LINES_LENGTH];

	// initialize matrix_lines
	for (int i = 0; i < MATRIX_LINES_LENGTH; i++) {
		matrix_lines[i].cur_length = -1;
	}

	// main loop
	while (1) {
		// for every matrix line
		for(int i = 0; i<MATRIX_LINES_LENGTH; i++) {
			struct matrix_line *ml = matrix_lines + i;
			// create new tail (old tail vanished)
			if (ml->cur_length == -1) {
				ml->head_x = getRandom() % SCREEN_WIDTH;
				ml->head_y = getRandom() % SCREEN_HEIGHT - 1;
				ml->length = getRandom() % MAX_LINE_LENGTH + 3;
				ml->cur_length = 0;
			}
			// set new char
			if (ml->head_y < SCREEN_HEIGHT-1) {
				ml->head_y++;
				char chr;
				int chrpos = ml->head_x - nickname_posx;
				if (nickname_action == NICKNAME_ACTION_SHOW
						&& ml->head_y == nickname_posy
						&& chrpos >= 0 && chrpos < nickname_len) {
					// show the nickname
					chr = GLOBAL(nickname)[chrpos];
				} else {
					chr = getRandom() % 95 + 33;
				}
				DoChar(ml->head_x * FONT_WIDTH, ml->head_y * FONT_HEIGHT, chr);
				ml->cur_length++;
			}
			// remove char (when length or bottom is reached)
			if (ml->cur_length > ml->length || ml->head_y >= SCREEN_HEIGHT-1) {
				int chrpos = ml->head_x - nickname_posx;
				if ( ! (nickname_action == NICKNAME_ACTION_SHOW
						&& (ml->head_y - ml->cur_length) == nickname_posy
						&& chrpos >= 0 && chrpos < nickname_len
						)) {
					// only delete, if it's not the nickname
					DoChar(ml->head_x * FONT_WIDTH, (ml->head_y - ml->cur_length) * FONT_HEIGHT, ' ');
				}
				ml->cur_length--;
			}
		}
		lcdDisplay();
		// show and hide nickname
		ticks_until_next_nickname_action--;
		if (ticks_until_next_nickname_action <= 0) {
			switch (nickname_action) {
			case NICKNAME_ACTION_SHOW:
				nickname_action = NICKNAME_ACTION_HIDE;
				ticks_until_next_nickname_action = TICKS_HIDE_NICKNAME;
				// new nickname_pos
				nickname_posx = getRandom() % (SCREEN_WIDTH - nickname_len);
				nickname_posy = getRandom() % SCREEN_HEIGHT;
				break;
			case NICKNAME_ACTION_HIDE:
				nickname_action = NICKNAME_ACTION_SHOW;
				ticks_until_next_nickname_action = TICKS_SHOW_NICKNAME;
				break;
			}
		}
		// Exit on any key
		int key = getInputRaw();
		if(key!= BTN_NONE)
			return;
		// sleep and process queue (e. g. meshnetwork)
		delayms_queue_plus(90,0);
	};
}
