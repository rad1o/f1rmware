/* bricks.c - provided by briks <briks@riseup.net> */

#include "basic/basic.h"
#include "usetable.h"

#define SCREEN_WIDTH  96
#define SCREEN_HEIGHT 67
#define FIELD_WIDTH   8
#define FIELD_HEIGHT  7
#define BRICK_WIDTH   11
#define BRICK_HEIGHT  4
#define BRICK_SPACING 1

#define PADDLE_WIDTH  20
#define PADDLE_Y      66
#define PADDLE_SPEED  3

#define PAUSE_INITIAL 30

#define LIVES_INITIAL 5

#define LEVELS        3

int levels[LEVELS][FIELD_HEIGHT][FIELD_WIDTH] = {
	{
		{0,0,0,0,0,0,0,0},
		{0,1,1,0,0,1,1,0},
		{0,1,1,0,0,1,1,0},
		{0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0},
		{0,1,0,0,0,0,1,0},
		{0,0,1,1,1,1,0,0}
	},
	{
		{0,1,0,1,0,1,0,1},
		{1,0,1,0,1,0,1,0},
		{0,1,0,1,0,1,0,1},
		{1,0,1,0,1,0,1,0},
		{0,1,0,1,0,1,0,1},
		{1,0,1,0,1,0,1,0},
		{0,0,0,0,0,0,0,0}
	},
	{
		{1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1},
		{0,0,0,0,0,0,0,0}
	}
};

int playLevel(int levelNo, int pause);
void drawBricks(int bricks[FIELD_HEIGHT][FIELD_WIDTH]);
void drawPxChk(int x, int y, int color);
void drawBall(int ball[2], int color);
void drawPaddle(int paddleX, int color);
int fieldIsCleared(int bricks[FIELD_HEIGHT][FIELD_WIDTH]);
int abs(int x);

void ram(void) {

	lcdClear();
	lcdPrintln("");
	lcdPrintln("");
	lcdPrintln("    BRICKS");
	lcdPrintln("");
	lcdPrintln("");
	lcdPrintln("");
	lcdPrintln("   by briks");
	lcdRefresh();
	delayms(1000);

	int pause = PAUSE_INITIAL;
	for (int i = 1; true; i++) {
		lcdClear();
		lcdPrintln("");
		lcdPrintln("");
		lcdPrintln("");
		lcdPrintln("    Level");
		lcdPrintln("");
		lcdPrint("      ");
		lcdPrintln(IntToStr(i, 2, 0));
		lcdRefresh();
		delayms(1000);		
		if (playLevel(i % LEVELS, pause) == 0) {
			return;
		} 
		pause = pause - (pause / 4); // shorten pause (increases speed)
	}
}

int playLevel(int levelNo, int pause) {

	lcdClear();

	// load level
	int bricks[FIELD_HEIGHT][FIELD_WIDTH];
	memcpy(bricks, levels[levelNo], sizeof(bricks));

	// initialisation
	int ball[2] = {SCREEN_WIDTH / 2, FIELD_HEIGHT * (BRICK_HEIGHT + BRICK_SPACING) + 1};
	int direction[2] = {1,1};
	int paddleX = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;
	int lives = LIVES_INITIAL;

	drawBricks(bricks);

	while ( 1 ) {

		// ball
		drawBall(ball, 0);
		ball[0] += direction[0];
		ball[1] += direction[1];

		// paddle / user input
		drawPaddle(paddleX, 0);
		int key = getInputRaw();
		switch (key) {
			case BTN_ENTER:
				// exit
				return 0;
			case BTN_LEFT:
				paddleX -= PADDLE_SPEED;
				if (paddleX < 0)
					paddleX = 0;
				break;
			case BTN_RIGHT:
				paddleX += PADDLE_SPEED;
				if (paddleX + PADDLE_WIDTH > SCREEN_WIDTH)
					paddleX = SCREEN_WIDTH - PADDLE_WIDTH;
				break;
		}
		drawPaddle(paddleX, 1);

		// collisions

		// bricks
		int x = ball[0] / ((BRICK_WIDTH + BRICK_SPACING));
		int y = ball[1] / ((BRICK_HEIGHT + BRICK_SPACING));
		if (0 <= x && x < FIELD_WIDTH && 0 <= y && y < FIELD_HEIGHT) {
			if (bricks[y][x] == 1) {
				// collision with brick
				int xRel = ball[0] - x * (BRICK_WIDTH + BRICK_SPACING);
				int yRel = ball[1] - y * (BRICK_HEIGHT + BRICK_SPACING);
				if (xRel == 0 || xRel == BRICK_WIDTH)
					direction[0] *= -1; // hit top or bottom
				if (yRel == 0 || yRel == BRICK_HEIGHT)
					direction[1] *= -1; // hit left or right
				bricks[y][x] = 0;
				if (fieldIsCleared(bricks))
					return 1; // next level
			}
		}

		// paddle / bottom
		if (direction[1] > 0) {
			// moving to the bottom 
			if (ball[1] >= PADDLE_Y) {
				if (paddleX <= ball[0] && ball[0] <= paddleX + PADDLE_WIDTH) {
					// collision with paddle
					direction[1] = - abs(direction[1]);
					if (key == BTN_LEFT)
						direction[0] = -2;
					else if (key == BTN_RIGHT)
						direction[0] = 2;
					else
						direction[0] = (direction[0] > 0) ? 1 : -1;
				}
				else {
					// ball lost
					lives--;
					if (lives == 0) {
						lcdClear();
						lcdPrintln("");
						lcdPrintln("");
						lcdPrintln("");
						lcdPrintln("");
						lcdPrintln("  GAME OVER");
						lcdRefresh();
						delayms(2000);
						return 0;
					}
					ball[0] = SCREEN_WIDTH / 2;
					ball[1] = FIELD_HEIGHT * (BRICK_HEIGHT + BRICK_SPACING) + 1;
					direction[0] = (paddleX + PADDLE_WIDTH / 2 < ball[0]) ? -1 : 1;
					direction[1] = 1;
				}
			}
		}

		// walls

		if (ball[1] <= 0)
			direction[1] = abs(direction[1]);
		if (ball[0] <= 0)
			direction[0] = abs(direction[0]);
		else if (ball[0] >= SCREEN_WIDTH - 1)
			direction[0] = - abs(direction[0]);

		drawBricks(bricks);
		drawBall(ball, 1);

		lcdRefresh();
		delayms(pause);
	}
	return 0;
}

void drawBricks(int bricks[FIELD_HEIGHT][FIELD_WIDTH]) {
	for (int x = 0; x < FIELD_WIDTH; x++)
		for (int y = 0; y < FIELD_HEIGHT; y++)
			for (int i = 0; i < BRICK_WIDTH; i++)
				for (int j = 0; j < BRICK_HEIGHT; j++)
					lcdSetPixel(x * (BRICK_WIDTH + BRICK_SPACING) + i, y * (BRICK_HEIGHT + BRICK_SPACING) + j, bricks[y][x]);
}

void drawPxChk(int x, int y, int color) {
	if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
		return;
	lcdSetPixel(x, y, color);
}

void drawBall(int ball[2], int color) {
	drawPxChk(ball[0] - 1, ball[1] - 1, color);
	drawPxChk(ball[0], ball[1] - 1, color);
	drawPxChk(ball[0] + 1, ball[1] - 1, color);
	drawPxChk(ball[0] - 1, ball[1], color);
	drawPxChk(ball[0], ball[1], color);
	drawPxChk(ball[0] + 1, ball[1], color);
	drawPxChk(ball[0] - 1, ball[1] + 1, color);
	drawPxChk(ball[0], ball[1] + 1, color);
	drawPxChk(ball[0] + 1, ball[1] + 1, color);
}

void drawPaddle(int paddleX, int color) {
	for (int x = 0; x < PADDLE_WIDTH; x++) {
		lcdSetPixel(paddleX + x, PADDLE_Y, color);
		lcdSetPixel(paddleX + x, PADDLE_Y + 1, color);
	}
}

int fieldIsCleared(int bricks[FIELD_HEIGHT][FIELD_WIDTH]) {
	for (int x = 0; x < FIELD_WIDTH; x++)
		for (int y = 0; y < FIELD_HEIGHT; y++)
			if (bricks[y][x] == 1)
				return 0;
	return true;
}

int abs(int x) {
	if (x < 0)
		return x * -1;
	return x;
}


