#include "basic/basic.h"
#include "basic/random.h"
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
#define TIME_PER_MOVE 500

typedef struct {
	int8_t x;
	int8_t y;
} point;

typedef struct {
	point startpoint;
	point endpoint;
	size_t starthn; // index of the half nibble (2 bits) where we start
	size_t endhn;
	int8_t data[NIBBLECOUNT]; // TODO: optimize to a power of 2
} vringpbuf; // variable (but limited) size ring buffer for points

int8_t getHalfNibble (int8_t *data, int index);
void setHalfNibble (int8_t *data, int index, int8_t value);
void shiftBuf (vringpbuf *buf, int8_t nextDir);
void growBuf (vringpbuf *buf, int8_t nextDir);
void shiftPoint (point *p, int8_t direction);

void initSnake ();
size_t getLength (vringpbuf* who);

// drawing functions in game coordinates
void drawPixelBlock (int8_t x, int8_t y, bool* img);
void setGamePixel (int8_t x, int8_t y, bool color);
void drawFood (int8_t x, int8_t y);
bool getGamePixel (int8_t x, int8_t y);

vringpbuf snake;
point bacon;

#define DIRECTION_RIGHT 0
#define DIRECTION_UP 1
#define DIRECTION_LEFT 2
#define DIRECTION_DOWN 3
int8_t direction;

int8_t i,j ;

void ram (void) {
	int key = getInputRaw(), button;
	lcdClear();
	initSnake ();
	while (1) {	
		switch (key) {
			case BTN_ENTER:
				// exit
				return;
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
				//Default: No keystroke received. Assuming last keystroke.
				
		}
		point newendpoint = snake.endpoint;
		shiftPoint (&newendpoint, direction);
		bool resetBacon = false;
		if (newendpoint.x == bacon.x && newendpoint.y == bacon.y) {
			growBuf (&snake, direction);
			resetBacon = true;
		} else {
			setGamePixel(snake.startpoint.x, snake.startpoint.y, 0);
			shiftBuf (&snake, direction);
		}

		if (getGamePixel(snake.endpoint.x, snake.endpoint.y))
			break;
		setGamePixel(snake.endpoint.x, snake.endpoint.y, 1);

		while (resetBacon) {
			bacon.x = getRandom() % GAME_WIDTH;
			bacon.y = getRandom() % GAME_HEIGHT;
			if (!getGamePixel(bacon.x, bacon.y))
				resetBacon = false;
		}
		
		drawFood (bacon.x, bacon.y);
		lcdRefresh();
		key = BTN_NONE;
		
		for (i=0; i < ( TIME_PER_MOVE / 50 ); ++i) {
			delayms(50);
			if ((button = getInputRaw()) != BTN_NONE)
				key = button;
		}

	}
	
	// Game ended, show results
	lcdClear();
	lcdNl();
	lcdPrintln("Game Over");
	lcdPrintln("Your score:");
	lcdPrintInt(getLength(&snake) - INITIAL_LENGTH);
	lcdRefresh();	
	delayms(500);
	while(getInputRaw() == BTN_NONE)
		delayms(25);

}

size_t getLength (vringpbuf* who) {
	size_t diff = who->endhn - who->starthn; 
	if (diff>0) {
		return diff + 1;
	} else {
		return diff + (NIBBLECOUNT + 1);
	}
}

int8_t getHalfNibble (int8_t *data, int index) {
	int8_t ret = data[index/4];
	return (ret >> (2*(index % 4))) & 3;
}

void setHalfNibble (int8_t *data, int index, int8_t value) {
	data[index/4] = ( data[index/4] & ~(3<<(2*(index%4))) ) | ((value&3)<<(2*(index%4)));
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
	bacon.x = getRandom() % GAME_WIDTH;
	bacon.y = getRandom() % GAME_HEIGHT;
	direction = DIRECTION_RIGHT;
	for (i=0; i<=2; i++) {
		setHalfNibble(snake.data, i, DIRECTION_RIGHT);
	}
}

void drawPixelBlock (int8_t x, int8_t y, bool* img) {
	int c = 0;
	for (i=x*BLOCK_SIZE; i<(x+1)*BLOCK_SIZE; i++) {
		for (j=y*BLOCK_SIZE; j<(y+1)*BLOCK_SIZE; j++) {
			lcdSetPixel (c,50,img[c]);
			lcdSetPixel (i,j,img[c++]);
		}
	}
}

void setGamePixel (int8_t x, int8_t y, bool color) {
	for (i=x*BLOCK_SIZE; i<(x+1)*BLOCK_SIZE; i++) {
		for (j=y*BLOCK_SIZE; j<(y+1)*BLOCK_SIZE; j++) {
			lcdSetPixel (i,j,color);
		}
	}
}

bool getGamePixel (int8_t x, int8_t y) {
	return lcdGetPixel (x*BLOCK_SIZE, y*BLOCK_SIZE);
}

void drawFood (int8_t x, int8_t y) {
#if BLOCK_SIZE == 4
	static bool food [] = {
		0, 1, 1, 0,
		1, 0, 0, 1,
		1, 0, 0, 1,
		0, 1, 1, 0
	};
	drawPixelBlock (x,y,food);
#else
#warning food will be drawn as full game pixel because food art is undefined for such BLOCK_SIZE
	setGamePixel (x,y,1);
#endif
}

