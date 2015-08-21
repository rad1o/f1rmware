#include <r0ketlib/display.h>
#include <r0ketlib/render.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include "usetable.h"

#define TILE_SIZE	2
#define SIZE_X 		(RESX/TILE_SIZE)
#define SIZE_Y      ((RESY-12)/TILE_SIZE)
#define NUM_PLAYERS	2

char state[SIZE_Y][SIZE_X];
char playersAlive[NUM_PLAYERS];
int playersPoints[NUM_PLAYERS];
int currentPlayer;

char colorLUT[] = {
	0b11100000,
	0b00011100,
	0b00000011,
	0b00000000
};

int position[][2] = {
	{ 0         , 0          },
	{ SIZE_X - 1, SIZE_Y - 1 },
	{ 0         , SIZE_Y - 1 },
	{ SIZE_X - 1, 0          }
};

struct point {
	char x;
	char y;
	char valid;
};

void generateRandom(void);
void renderState(void);
void renderHelp(void);
void renderPoints(void);
void handleInput();
void fill(char color, int player);
void nextPlayer(void);
void renderTile(int x, int y, char color);

void ram(void) {
	currentPlayer = 0;
	
	for(int i = 0; i < NUM_PLAYERS; i++) {
		playersAlive[i] = 1;
		playersPoints[i] = 1;
	}

	lcdClear();
	lcdDisplay();

	generateRandom();

	while(1) {
		renderState();
		renderHelp();
		renderPoints();
		lcdDisplay();
		delayms(50);

		handleInput();
		lcdClear();
	}
}

void generateRandom(void) {
	for(int y = 0; y < SIZE_Y; y++) {
		for(int x = 0; x < SIZE_X; x++) {
			state[y][x] = getRandom() % 4;
		}
	}
}

void renderState(void) {
	for(int y = 0; y < SIZE_Y; y++) {
		for(int x = 0; x < SIZE_X; x++) {
			renderTile(x, y, state[y][x]);
		}
	}
}

void renderHelp(void) {
	/* bitmaps are boring */
	for(int i = 0; i < 4; i++) {
		lcdSetPixel(4, RESY-12+i, colorLUT[0]);
		lcdSetPixel(5, RESY-12+i, colorLUT[0]);
		lcdSetPixel(6, RESY-12+i, colorLUT[0]);
		lcdSetPixel(7, RESY-12+i, colorLUT[0]);
	}

	for(int i = 0; i < 4; i++) {
		lcdSetPixel(0, RESY-8+i, colorLUT[1]);
		lcdSetPixel(1, RESY-8+i, colorLUT[1]);
		lcdSetPixel(2, RESY-8+i, colorLUT[1]);
		lcdSetPixel(3, RESY-8+i, colorLUT[1]);
	}

	for(int i = 0; i < 4; i++) {
		lcdSetPixel(8, RESY-8+i, colorLUT[2]);
		lcdSetPixel(9, RESY-8+i, colorLUT[2]);
		lcdSetPixel(10, RESY-8+i, colorLUT[2]);
		lcdSetPixel(11, RESY-8+i, colorLUT[2]);
	}
	
	for(int i = 0; i < 4; i++) {
		lcdSetPixel(4, RESY-4+i, colorLUT[3]);
		lcdSetPixel(5, RESY-4+i, colorLUT[3]);
		lcdSetPixel(6, RESY-4+i, colorLUT[3]);
		lcdSetPixel(7, RESY-4+i, colorLUT[3]);
	}

	DoChar(14, RESY-9, '1' + currentPlayer);
}

void renderPoints(void) {
	DoString(25, RESY-9, IntToStr(playersPoints[0], 4, F_LONG));
	DoString(60, RESY-9, IntToStr(playersPoints[1], 4, F_LONG));
}

void handleInput() {

	char color = -1;

	while(1) {
		int input = getInput();
		if(input & BTN_UP) {
			color = 0;
		}
		else if(input & BTN_LEFT) {
			color = 1;
		}
		else if(input & BTN_RIGHT) {
			color = 2;
		}
		else if(input & BTN_DOWN) {
			color = 3;
		}
		else {
			continue;
		}

		char oldColor = state[position[currentPlayer][1]][position[currentPlayer][0]];

		if(oldColor != color) {
			break;
		}
	}

	fill(color, currentPlayer);

	nextPlayer();
}

int fillIterative(int x, int y, char color, char oldColor) {
	static const int stack_size = 4000;
	struct point stack[stack_size];
	
	for(int i = 0; i < stack_size; i++) {
		stack[i].valid = 0;
	}
	
	stack[0].x = x;
	stack[0].y = y;
	stack[0].valid = 1;
	int current_stack_size = 1;
	
	int num_changed_tiles = 0;

	while(current_stack_size) {
		if(!stack[current_stack_size - 1].valid) {
			continue;
		}

		int cx = stack[current_stack_size - 1].x;
		int cy = stack[current_stack_size - 1].y;
		stack[current_stack_size - 1].valid = 0;
		current_stack_size--;
		
		if(cx < 0 || cx >= SIZE_X || cy < 0 || cy >= SIZE_Y) {
			continue;
		}

		int ccolor = state[cy][cx];

		if(ccolor == oldColor) {
			state[cy][cx] = color;
			num_changed_tiles++;

			for(int i = 0; i < NUM_PLAYERS; i++) {
				if(i == currentPlayer) {
					continue;
				}

				int playerStartX = position[i][0];
				int playerStartY = position[i][1];

				if(cx == playerStartX && cy == playerStartY) {
					playersAlive[i] = 0;
					playersPoints[i] = 0;
				}
			}

			if(state[cy][cx - 1] == oldColor) {
				current_stack_size++;
				stack[current_stack_size - 1].x = cx - 1;
				stack[current_stack_size - 1].y = cy;
				stack[current_stack_size - 1].valid = 1;
			}

			if(state[cy][cx + 1] == oldColor) {
				current_stack_size++;
				stack[current_stack_size - 1].x = cx + 1;
				stack[current_stack_size - 1].y = cy;
				stack[current_stack_size - 1].valid = 1;
			}

			if(state[cy - 1][cx] == oldColor) {
				current_stack_size++;
				stack[current_stack_size - 1].x = cx;
				stack[current_stack_size - 1].y = cy - 1;
				stack[current_stack_size - 1].valid = 1;
			}

			if(state[cy + 1][cx] == oldColor) {
				current_stack_size++;
				stack[current_stack_size - 1].x = cx;
				stack[current_stack_size - 1].y = cy + 1;
				stack[current_stack_size - 1].valid = 1;
			}
		}
	}

	return num_changed_tiles;
}

void fill(char color, int player) {
	int startX = position[player][0];
	int startY = position[player][1];

	fillIterative(startX, startY, color, state[startY][startX]);
	int points = fillIterative(startX, startY, -1, state[startY][startX]);
	fillIterative(startX, startY, color, -1);
	playersPoints[player] = points;
}

void nextPlayer(void) {
	do {
		currentPlayer++;

		if(currentPlayer == NUM_PLAYERS) {
			currentPlayer = 0;
		}
	} while(!playersAlive[currentPlayer]);
}

void renderTile(int x, int y, char color) {
	for(int iy = 0; iy < TILE_SIZE; iy++) {
		for(int ix = 0; ix < TILE_SIZE; ix++) {
			lcdSetPixel((x * TILE_SIZE) + ix, (y * TILE_SIZE) + iy, colorLUT[(int)color]);
		}
	}
}

