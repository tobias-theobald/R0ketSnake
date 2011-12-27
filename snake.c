#include "basic/basic.h"

#include "lcd/render.h"
#include "lcd/print.h"
#include "lcd/display.h"

#include "usetable.h"

#define SCREEN_WIDTH  96
#define SCREEN_HEIGHT 67
#define SCREEN_SIZE ( SCREEN_WIDTH * SCREEN_HEIGHT )
#define BLOCK_SIZE 4
#define GAME_WIDTH ( SCREEN_WIDTH / BLOCK_SIZE )
#define GAME_HEIGHT ( SCREEN_HEIGHT / BLOCK_SIZE )
#define GAME_SIZE ( GAME_WIDTH * GAME_HEIGHT )

#define INITIAL_LENGTH 4

typedef struct {
	int8_t x;
	int8_t y;
} point;

typedef struct {
	point startpoint;
	size_t starthn; // index of the half nibble (2 bits) where we start
	int8_t length;
	int8_t[GAME_SIZE / 4] data; // TODO: optimize to a power of 2
} vringpbuf; // variable (but limited) size ring buffer for points

int8_t shiftBuf (vringpbuf *buf, int8_t nextDir);
int8_t growBuf (vringpbuf *buf, int8_t nextDir);


vringpbuf snake;
#define DIRECTION_RIGHT 0
#define DIRECTION_UP 1
#define DIRECTION_LEFT 2
#define DIRECTION_DOWN 3
int8_t direction;

int8_t i;

void initSnake (vringpbuf *);

void game (void) {
	lcdClear();
	initSnake ();
	while (1) {
		
		int key = getInputRaw();
		switch (key) {
			case BTN_ENTER:
				// exit
				return 0;
			case BTN_RIGHT:
				if (direction != DIRECTION_LEFT)
					direction = DIRECTION_RIGHT;
			break;
			case BTN_UP:
				if (direction != DIRECTION_DOWN)
					direction = DIRECTION_UP;
			break;
			case BTN_LEFT:
				if (direction != DIRECTION_RIGHT)
					direction = DIRECTION_LEFT;
			break;
			case BTN_DOWN:
				if (direction != DIRECTION_UP)
					direction = DIRECTION_DOWN;
			break;
			default:
				// strange things happen!
				return 1;
		}
	}
}

int8_t getHalfNibble (int8_t *data, int index) {
	int8_t ret = data[index/4];
	return (ret >> (index % 4)) & 3;
}

void setHalfNibble (int8_t *data, int index, int8_t value) {
	//TODO
}

int8_t shiftBuf (vringpbuf *buf, int8_t nextDir) {
	//TODO
}

int8_t growBuf (vringpbuf *buf, int8_t nextDir) {
	//TODO
}

void initSnake (void) {
	//TODO
}

