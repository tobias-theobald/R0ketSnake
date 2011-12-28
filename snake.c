#include "basic/basic.h"
#include "basic/random.h"
#include "lcd/render.h"
#include "lcd/print.h"
#include "lcd/display.h"
#include "funk/nrf24l01p.h"

#include "usetable.h"

#define SCREEN_WIDTH  96
#define SCREEN_HEIGHT 68
#define SCREEN_SIZE ( SCREEN_WIDTH * SCREEN_HEIGHT )
#define BLOCK_SIZE 4
#define GAME_WIDTH ( SCREEN_WIDTH / BLOCK_SIZE )
#define GAME_HEIGHT ( SCREEN_HEIGHT / BLOCK_SIZE )
#define GAME_SIZE ( GAME_WIDTH * GAME_HEIGHT )
#define NIBBLECOUNT ( GAME_SIZE / 4 )

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

void singlePlayer (void);
void multiPlayer (void);

int8_t getHalfNibble (int8_t *data, int index);
void setHalfNibble (int8_t *data, int index, int8_t value);
void shiftBuf (vringpbuf *buf, int8_t nextDir);
void growBuf (vringpbuf *buf, int8_t nextDir);
void shiftPoint (point *p, int8_t direction);

void initSnake (void);
void initSnake2 (void);
size_t getLength (vringpbuf* who);

uint8_t initRadioAndLookForGames(int timeout); // returns -1 if timeout (no host found), gameID (bit 2-5), bacon x (6-10) and bacon y (11-15) else
uint8_t switchToHostModeAndWaitForClients(int timeout); // returns -1 if timeout (no host found), gameID (bit 2-5), bacon x (6-10) and bacon y (11-15) else
<<<<<<< HEAD
uint8_t receiveKeyPressed(int timeout, uint8_t gameID); // to be used by host in wait loop
void sendKeyPressed(uint8_t keyPressed, int timeout, uint8_t gameID); // to be used by client in wait loop
//TODO: make sendKeyPressed return the last key pressed while waiting
void receiveMove(uint8_t * display, uint8_t * baconx, uint8_t * bacony, int timeout, int loops, uint8_t gameID); // to be used by client when game should be received (display must be uint8_t[52], baconx and y uint8_t)
//TODO: make receiveMove return the last key pressed while waiting
void sendMove(uint8_t * display, uint8_t baconx, uint8_t bacony, int timeout, int loops, uint8_t gameID); // to be used by host when game display must be sent (display must be uint8_t[52])
//TODO: make sendMove return the last key pressed while waiting
=======
uint8_t receiveKeyPressed(int timeout); // to be used by host in wait loop
void sendKeyPressed(uint8_t keyPressed, int timeout); // to be used by client in wait loop
void receiveMove(uint8_t * display, uint8_t * baconx, uint8_t * bacony, int timeout); // to be used by client when game should be received (display must be uint8_t[52], baconx and y uint8_t)
void sendMove(uint8_t * display, uint8_t baconx, uint8_t bacony, int timeout); // to be used by host when game display must be sent (display must be uint8_t[52])
>>>>>>> 0da73a078eb28495f4d6c1043bcfd4b5ecd8ab27
uint8_t getBits(uint8_t mask, uint8_t bit, uint8_t len); // internal; returns bits bit to (bit+len-1) from mask on the rightmost side 

// drawing functions in game coordinates
void drawPixelBlock (int8_t x, int8_t y, bool* img);
void setGamePixel (int8_t x, int8_t y, bool color);
void drawFood (int8_t x, int8_t y);
bool getGamePixel (int8_t x, int8_t y);
void fillBlock (int8_t x, int8_t y, int8_t x2, int8_t y2, bool color);

vringpbuf snake;
vringpbuf snake2;
point bacon;

#define DIRECTION_RIGHT 0
#define DIRECTION_UP 1
#define DIRECTION_LEFT 2
#define DIRECTION_DOWN 3
int8_t direction;
int8_t direction2;

int8_t i,j ;

void ram (void) {
	int key = BTN_NONE;
	while (key != BTN_ENTER) {
		if (key == BTN_UP) {
			singlePlayer();
			delayms(100); // agains double-eval of a button
		} else if (key == BTN_DOWN) {
			multiPlayer();
			delayms(100);
		}
		key = BTN_NONE;
		lcdClear();
		lcdPrintln("R0ketSnake");
		lcdNl();
		lcdPrintln("  UP 1 Player");
		lcdPrintln("DOWN 2 Players");
		lcdPrintln("ENTER Quit");
		lcdRefresh();

		while((key = getInputRaw()) == BTN_NONE)
			delayms(25);
	}
}

void singlePlayer (void) {
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
	lcdPrintInt(getLength(&snake) - INITIAL_LENGTH + 1);
	lcdNl();
	lcdPrintln("Press any key");
	lcdPrintln("to continue.");
	lcdRefresh();
	delayms(500);
	while(getInputRaw() == BTN_NONE)
		delayms(25);

}

void multiPlayer(void) {
	lcdClear();
	lcdPrintln(" looking for ");
	lcdPrintln("  existing   ");
	lcdPrintln("   games...  ");
	if (initRadioAndLookForGames(1000)) { // game found
		// act as controller and screen
		client();
	} else { // no game found
		// PRINT: awaiting connection
		if (switchToHostModeAndWaitForClients (10000)) { // wait for 10 secs
			//game found
			//TODO: start game as host
		} else {
			lcdClear();
			lcdPrintln("nobody joined");
		}
	}
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
}

void client (void) {
	uint8_t dsp [GAME_SIZE/8];
	uint8_t key;
	while(1) {
		receiveMove(dsp, &(bacon.x), &(bacon.y), TIME_PER_MOVE/16);
		paintDisplay (dsp);
		drawFood (bacon.x, bacon.y);
		key = getInputRaw();
		sendKeyPressed (key, TIME_PER_MOVE/16);
	}
}

// returns 0 if timeout (no host found), gameID (!= 0) else
uint8_t initRadioAndLookForGames(int timeout) {

	uint8_t macListen[5] = {0x04, 0x08, 0x02, 0x06, 0x00};
    	struct NRF_CFG configListen;
	configListen.nrmacs=1;
	configListen.maclen[0] = 16;
	configListen.channel = 81;
	memcpy(configListen.mac0, macListen, 5);
	memcpy(configListen.txmac, macListen, 5);
	nrf_config_set(&configListen);

	// Broadcast Message format: 1 Byte
	// 00 01 02 03 04 05 06 07
	// 00-07: gameID
	uint8_t gameID;

	if (nrf_rcv_pkt_time(timeout, 1, &gameID) != 1) // wrong package length or nothing received
		return 0;
		
	// At this point, we know there is an open game. Let's join it.
	
	uint8_t macIngame[5] = {0x04, 0x08, 0x02, 0x06, gameID};
    	struct NRF_CFG configIngame;
	configIngame.nrmacs=1;
	configIngame.maclen[0] = 16;
	configIngame.channel = 81;
	memcpy(configIngame.mac0, macIngame, 5);
	memcpy(configIngame.txmac, macIngame, 5);
	nrf_config_set(&configIngame);
	
	uint8_t init = BTN_NONE;
	delayms(4);	
	nrf_snd_pkt_crc(1, &init);
	
	return gameID; 
}

// returns 0 if timeout, gameID (!= 0) else
uint8_t switchToHostModeAndWaitForClients(int timeout) {
	
	// Broadcast Message format: 1 Byte
	// 00 01 02 03 04 05 06 07
	// 00-07: gameID
	uint8_t gameID = 0, buf;

	for (i = 0; i < timeout / 16; ++i) {	
		while (!gameID)
			gameID = getRandom();

		uint8_t macListen[5] = {0x04, 0x08, 0x02, 0x06, 0x00};
    		struct NRF_CFG configListen;
		configListen.nrmacs=1;
		configListen.maclen[0] = 16;
		configListen.channel = 81;
		memcpy(configListen.mac0, macListen, 5);
		memcpy(configListen.txmac, macListen, 5);
		nrf_config_set(&configListen);
		nrf_snd_pkt_crc(1, &gameID);
		
		uint8_t macIngame[5] = {0x04, 0x08, 0x02, 0x06, gameID};
	    	struct NRF_CFG configIngame;
		configIngame.nrmacs=1;
		configIngame.maclen[0] = 16;
		configIngame.channel = 81;
		memcpy(configIngame.mac0, macIngame, 5);
		memcpy(configIngame.txmac, macIngame, 5);
		nrf_config_set(&configIngame);
	
		if (nrf_rcv_pkt_time(32, 1, &buf) == 1 && buf == 0)
			return gameID;

		gameID = 0;
	}
	return 0;
}

// to be used by host in wait loop
uint8_t receiveKeyPressed(int timeout) {
	uint8_t buf;
	if (nrf_rcv_pkt_time(timeout, 1, &buf) == 1)
		delayms(timeout);
	return buf;
}

// to be used by client in wait loop
void sendKeyPressed(uint8_t keyPressed, int timeout) {
	nrf_snd_pkt_crc(1, &keyPressed);
	delayms(timeout);
}

// to be used by client when game must be received (display must be uint8_t[52], bacony and y uint8_t)
void receiveMove(uint8_t * display, uint8_t * baconx, uint8_t * bacony, int timeout) {
	uint8_t buf[54];
	if (nrf_rcv_pkt_time(timeout, 54, buf) != 54)
		return;
	delayms(timeout);
	memcpy(display, buf, 52);
	*baconx = buf[52];
	*bacony = buf[53];
}

// to be used by host when game display must be sent (display must be uint8_t[52])
void sendMove(uint8_t * display, uint8_t baconx, uint8_t bacony, int timeout) {
	uint8_t buf[54];
	memcpy(buf, display, 52);
	buf[52] = baconx;
	buf[53] = bacony;
	nrf_snd_pkt_crc(54, buf);
	delayms(timeout);
}


uint8_t getBits(uint8_t mask, uint8_t bit, uint8_t len) {
	return (mask << bit) >> (8-len);
}

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

void initSnake (void) {
	snake.startpoint.x = GAME_WIDTH-4;
	snake.startpoint.y = 0;
	snake.endpoint.x = GAME_WIDTH-1;
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

void initSnake2 (void) {
	snake2.startpoint.x = 3;
	snake2.startpoint.y = GAME_HEIGHT-1;
	snake2.endpoint.x = 0;
	snake2.endpoint.y = GAME_HEIGHT-1;
	snake2.starthn = 0;
	snake2.endhn = 2;
	direction = DIRECTION_LEFT;
	for (i=0; i<=2; i++) {
		setHalfNibble(snake.data, i, DIRECTION_LEFT);
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

