#include <stdint.h>

#include <r0ketlib/config.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include "usetable.h"

#define MIN(A,B) (A < B ? A : B)
#define MAX(A,B) (A >= B ? A : B)
#define BETWEEN(X,START,END) (MIN(MAX(X, START), END))

#define MIN_NUM_STATES 4
#define MAX_NUM_STATES 24

#define NEXT_STATE(STATE) ((STATE + color_step) % max_color)
#define RAND_COLOR ((getRandom() % num_states) * color_step)

#define MIN_BLOCK_SIZE 2
#define MAX_BLOCK_SIZE 8
#define MAX_SIZE_X (RESX / MIN_BLOCK_SIZE)
#define MAX_SIZE_Y (RESY / MIN_BLOCK_SIZE)

#define GET_STATE(X,Y) (lcdGetPixel(X * block_size, Y * block_size))
#define GET_NEXT_STATE(X,Y) (new_buffer[X][Y])
#define SET_NEXT_STATE(X,Y,VAL) (new_buffer[X][Y] = VAL)

// states <-> colors
uint8_t num_states;
uint8_t max_color;
uint8_t color_step;

// pixels <-> blocks
uint8_t block_size;
uint8_t size_x;
uint8_t size_y;
uint8_t pad_x;
uint8_t pad_y;

uint8_t new_buffer[MAX_SIZE_X][MAX_SIZE_Y];
bool stop;

static void handle_input();
static void reset();
static void step();
static void draw_block(uint8_t i, uint8_t j, uint8_t color);
static void draw();

void ram(void)
{
    num_states = MIN_NUM_STATES;
    block_size = MIN_BLOCK_SIZE;
    stop = false;

    reset();
    while (!stop)
    {
	draw();
	handle_input();
	step();
    }
}

void handle_input()
{
    // TODO: quit?
    switch (getInputRaw()) {
    case BTN_UP:
	block_size = BETWEEN(block_size - 1, MIN_BLOCK_SIZE, MAX_BLOCK_SIZE);
	reset();
	break;
    case BTN_DOWN:
	block_size = BETWEEN(block_size + 1, MIN_BLOCK_SIZE, MAX_BLOCK_SIZE);
	reset();
	break;
    case BTN_LEFT:
	num_states = BETWEEN(num_states - 1, MIN_NUM_STATES, MAX_NUM_STATES);
	reset();
	break;
    case BTN_RIGHT:
	num_states = BETWEEN(num_states + 1, MIN_NUM_STATES, MAX_NUM_STATES);
	reset();
	break;
    case BTN_ENTER:
	stop = true;
	break;
    }
}

void reset()
{
    max_color = ((((UINT8_MAX / 2) * 2) / num_states) * num_states);
    color_step = (max_color / num_states);

    size_x = RESX / block_size;
    size_y = RESY / block_size;

    pad_x = (RESX - size_x * block_size) / 2;
    pad_y = (RESY - size_y * block_size) / 2;

    for (uint8_t i = 0; i < size_x; i++) {
	for (uint8_t j = 0; j < size_y; j++) {
	    SET_NEXT_STATE(i, j, RAND_COLOR);
	}
    }
    
    lcdFill(0);
}

void step()
{
    for (uint8_t i = 0; i < size_x; i++) {
	for (uint8_t j = 0; j < size_y; j++) {
	    uint8_t min_x = MAX(i - 1, 0);
	    uint8_t max_x = MIN(i + 1, size_x);

	    uint8_t min_y = MAX(j - 1, 0);
	    uint8_t max_y = MIN(j + 1, size_y);
	    
	    uint8_t next_state = NEXT_STATE(GET_STATE(i, j));

	    for (uint8_t x = min_x; x <= max_x; x++) {
		for (uint8_t y = min_y; y <= max_y; y++) {
		    // avoid iterating over itself
		    if (x == i && y == j) {
			continue;
		    }
		    // update color if neighbor has next color
		    if (GET_STATE(x, y) == next_state) {
			SET_NEXT_STATE(i, j, next_state);
			x = max_x + 1; y = max_y + 1; 
			break;
		    }
		}
	    }
	}
    }
}

void draw_block(uint8_t i, uint8_t j, uint8_t color)
{
    for (uint8_t dx = 0; dx < block_size; dx++) {
	for (uint8_t dy = 0; dy < block_size; dy++) {
            uint8_t x = i * block_size + dx + pad_x;
            uint8_t y = j * block_size + dy + pad_y;
	    lcdSetPixel(x, y, color);
	}
    }
}

void draw()
{
    for (uint8_t i = 0; i < size_x; i++) {
	for (uint8_t j = 0; j < size_y; j++) {
	    draw_block(i, j, GET_NEXT_STATE(i, j));
	}
    }
    lcdDisplay();
}
