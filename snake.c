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
#define NIBBLECOUNT ( GAME_SIZE / 4 )

#define INITIAL_LENGTH 4

typedef struct {
	int8_t x;
	int8_t y;
} point;

typedef struct {
	point startpoint;
	point endpoint;
	size_t starthn; // index of the half nibble (2 bits) where we start
	int8_t endhn;
	int8_t data[NIBBLECOUNT]; // TODO: optimize to a power of 2
} vringpbuf; // variable (but limited) size ring buffer for points

int8_t getHalfNibble (int8_t *data, int index);
void setHalfNibble (int8_t *data, int index, int8_t value);
void shiftBuf (vringpbuf *buf, int8_t nextDir);
void growBuf (vringpbuf *buf, int8_t nextDir);


vringpbuf snake;
#define DIRECTION_RIGHT 0
#define DIRECTION_UP 1
#define DIRECTION_LEFT 2
#define DIRECTION_DOWN 3
int8_t direction;

int8_t i;

void initSnake ();

int game (void) {
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
		lcdSetPixel(snake.startpoint.x, snake.startpoint.y, 0);
		shiftBuf (&snake, direction);
		lcdSetPixel(snake.endpoint.x, snake.endpoint.y, 1);
	}
}

int8_t getHalfNibble (int8_t *data, int index) {
	int8_t ret = data[index/4];
	return (ret >> (index % 4)) & 3;
}

void setHalfNibble (int8_t *data, int index, int8_t value) {
	data[index/4] = ( data[index/4] & ~(3<<(index%4)) ) | (value&3)<<(index%4);
}

void shiftPoint (point *p, int8_t direction) {
	switch (direction) {
		case DIRECTION_RIGHT:
			p->x = (p->x + 1) % GAME_WIDTH;
		break;
		case DIRECTION_UP:
			p->y = (p->y - 1 + GAME_HEIGHT) % GAME_HEIGHT;
		break;
		case DIRECTION_LEFT:
			p->x = (p->x - 1 + GAME_WIDTH) % GAME_WIDTH;
		break;
		case DIRECTION_DOWN:
			p->y = (p->y + 1) % GAME_HEIGHT;
		break;
		default:
			while (1);
			// ERROR! but what should I do about it?
	}
}

void shiftBuf (vringpbuf *buf, int8_t nextDir) {
	shiftPoint (&(buf->startpoint), getHalfNibble (buf->data, buf->starthn));
	buf->starthn = (buf->starthn + 1) % GAME_SIZE;
	growBuf (buf, nextDir);
}

void growBuf (vringpbuf *buf, int8_t nextDir) {
	buf->endhn = (buf->endhn + 1) % GAME_SIZE;
	setHalfNibble(buf->data, buf->endhn, nextDir);
	shiftPoint (&(buf->endpoint), nextDir);
}

void initSnake (void) {
	snake.startpoint.x = 0;
	snake.startpoint.y = 0;
	snake.endpoint.x = 3;
	snake.endpoint.y = 0;
	snake.starthn = 0;
	snake.endhn = 2;
	for (i=0; i<=2; i++) {
		setHalfNibble(snake.data, i, DIRECTION_RIGHT);
	}
}

