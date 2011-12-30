#include <sysinit.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "basic/basic.h"
#include "basic/config.h"
#include "basic/random.h"
#include "lcd/render.h"
#include "lcd/print.h"
#include "lcd/display.h"

#include "usetable.h"

#define SCREEN_WIDTH  96
#define SCREEN_HEIGHT 68
#define SCREEN_SIZE ( SCREEN_WIDTH * SCREEN_HEIGHT )
#define BLOCK_SIZE 4
#define GAME_WIDTH ( SCREEN_WIDTH / BLOCK_SIZE )
#define GAME_HEIGHT ( SCREEN_HEIGHT / BLOCK_SIZE )
#define GAME_SIZE ( GAME_WIDTH * GAME_HEIGHT )
#define NIBBLECOUNT ( GAME_SIZE / 4 )

#define DIRECTION_RIGHT 0
#define DIRECTION_UP 1
#define DIRECTION_LEFT 2
#define DIRECTION_DOWN 3

#define INITIAL_LENGTH 4
#define TIME_PER_MOVE 300

typedef struct {
	uint8_t x;
	uint8_t y;
} point;

typedef struct {
	point startpoint;
	point endpoint;
	size_t starthn; // index of the half nibble (2 bits) where we start
	size_t endhn;
	int8_t data[NIBBLECOUNT]; // not todo because fast enough: optimize to a power of 2
} vringpbuf; // variable (but limited) size ring buffer for points

// main function prototypes
void ram (void);
void main_snake (void);

// helper function prototypes
int8_t getHalfNibble (int8_t *data, int index);
void setHalfNibble (int8_t *data, int index, int8_t value);
void shiftBuf (vringpbuf *buf, int8_t nextDir);
void growBuf (vringpbuf *buf, int8_t nextDir);
void shiftPoint (point *p, int8_t direction);
void initSnake (vringpbuf* what, uint8_t sx, uint8_t sy, uint8_t ex, uint8_t ey, uint8_t direc);
size_t getLength (vringpbuf* who);
uint8_t getBits(uint8_t mask, uint8_t bit, uint8_t len); // internal; returns bits bit to (bit+len-1) from mask on the rightmost side 
//void memcopy(uint8_t * s1, const uint8_t * s2, size_t n);

// drawing game coordinates function prototypes 
void drawPixelBlock (int8_t x, int8_t y, bool* img);
void setGamePixel (int8_t x, int8_t y, bool color);
void drawFood (int8_t x, int8_t y);
bool getGamePixel (int8_t x, int8_t y);
void fillBlock (int8_t x, int8_t y, int8_t x2, int8_t y2, bool color);

// global variables
vringpbuf snake;
point bacon;
int8_t direction, i, j;

void main_snake (void) {
	bacon.x = getRandom() % GAME_WIDTH;
	bacon.y = getRandom() % GAME_HEIGHT;
	direction = DIRECTION_RIGHT;
	initSnake (&snake, GAME_WIDTH-4, 0, GAME_WIDTH-1, 0, DIRECTION_RIGHT);
	ram();
}

bool getIthBit (uint8_t byte, size_t i) {
	return (byte>>i)&1;
}

void setIthBit (uint8_t *byte, size_t i, bool value) {
	if (value)
		*byte = *byte | 1<<i;
	else
		*byte = *byte & ~(1<<i);
}

inline size_t twoD2OneD (size_t x, size_t y) {
	return y*GAME_WIDTH+x;
}

bool getPixelOnDsp (uint8_t *dsp, uint8_t x, uint8_t y) {
	int index = twoD2OneD(x,y);
	uint8_t theByte = dsp[index/8];
	return getIthBit (theByte, index%8);
}

void setPixelOnDsp (uint8_t *dsp, uint8_t x, uint8_t y, bool value) {
	int index = twoD2OneD(x,y);
	setIthBit (&(dsp[index/8]), index%8, value);
}

void paintDisplay (uint8_t *dsp) {
	for (i=0; i<GAME_HEIGHT; i++) {
		for (j=0; j<GAME_WIDTH; j++) {
			setGamePixel (j, i, getPixelOnDsp(dsp,j,i));
		}
	}
	lcdRefresh();
}

uint8_t getBits(uint8_t mask, uint8_t bit, uint8_t len) {
	return (mask << bit) >> (8-len);
}

/*void memcopy(uint8_t * s1, const uint8_t * s2, size_t n) {
	for (i = 0; i < n; ++i)
		*(s1+i) = *(s2+i);
}*/

void fillBlock (int8_t x, int8_t y, int8_t x2, int8_t y2, bool color) {
	for (i=x; i<=x2; i++)
		for (j=y; j<=y2; j++)
			lcdSetPixel (i,j,color);
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

void initSnake (vringpbuf* snake, uint8_t sx, uint8_t sy, uint8_t ex, uint8_t ey, uint8_t direc) {
	snake->startpoint.x = sx;//GAME_WIDTH-4;
	snake->startpoint.y = sy;//0;
	snake->endpoint.x = ex;//GAME_WIDTH-1;
	snake->endpoint.y = ey;//0;
	snake->starthn = 0;
	snake->endhn = 2;
	for (i=0; i<=2; i++) {
		setHalfNibble(snake->data, i, direc);
	}
}

void drawPixelBlock (int8_t x, int8_t y, bool* img) {
	int c = 0;
	for (i=y*BLOCK_SIZE; i<(y+1)*BLOCK_SIZE; i++) {
		for (j=x*BLOCK_SIZE; j<(x+1)*BLOCK_SIZE; j++) {
			lcdSetPixel (j,i,img[c++]);
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
	//upper left pixel must be 0!!
	bool food [] = {
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

