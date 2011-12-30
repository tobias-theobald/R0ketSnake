#include "funk/nrf24l01p.h"
#include "snake_shared.c"

void host (void);
void initSnake2 (void);

// radio function prototypes
uint8_t initRadioAndLookForGames(int timeout); // returns -1 if timeout (no host found), gameID (bit 2-5), bacon x (6-10) and bacon y (11-15) else
uint8_t switchToHostModeAndWaitForClients(int timeout); // returns -1 if timeout (no host found), gameID (bit 2-5), bacon x (6-10) and bacon y (11-15) else
uint8_t receiveKeyPressed(int timeout); // to be used by host in wait loop
void sendKeyPressed(uint8_t keyPressed, int timeout); // to be used by client in wait loop
void receiveMove(uint8_t * display, uint8_t * baconx, uint8_t * bacony, int timeout); // to be used by client when game should be received (display must be uint8_t[52], baconx and y uint8_t)
void sendMove(uint8_t * display, uint8_t baconx, uint8_t bacony, int timeout); // to be used by host when game display must be sent (display must be uint8_t[52])

struct NRF_CFG config = {
    .channel= 81,
    .txmac= "\x04\x08\x0c\x10\xff",
    .nrmacs=1,
    .mac0=  "\x04\x08\x0c\x10\xff",
    .maclen ="\x20",
};

vringpbuf snake2;
point bacon;
int8_t direction2;

void ram(void) {
	initRadioAndLookForGames(0);
	if (switchToHostModeAndWaitForClients (10000)) { // wait for 10 secs
		//someone joined
		host();
	} else {
		lcdClear();
		lcdPrintln("nobody joined");
		lcdRefresh();
		delayms(500);
	}
}

inline void host (void) {
	uint8_t key = BTN_RIGHT, button;
	uint8_t p2key = BTN_LEFT;
	lcdClear();
	initSnake ();
	initSnake2 ();
	uint8_t dsp [GAME_SIZE/8];
	for (i=0; i<GAME_SIZE/8; i++)
		dsp[i] = 0;
	while (1) {
		sendMove (dsp, bacon.x, bacon.y, 0);
		switch (p2key) {
			case BTN_ENTER:
				// exit
				return;
			case BTN_RIGHT:
				if (direction2 != DIRECTION_LEFT)
					direction2 = DIRECTION_RIGHT;
			break;
			case BTN_UP:
				if (direction2 != DIRECTION_DOWN)
					direction2 = DIRECTION_UP;
			break;
			case BTN_LEFT:
				if (direction2 != DIRECTION_RIGHT)
					direction2 = DIRECTION_LEFT;
			break;
			case BTN_DOWN:
				if (direction2 != DIRECTION_UP)
					direction2 = DIRECTION_DOWN;
			break;
				//Default: No keystroke received. Assuming last keystroke.
		}
		point newendpoint = snake2.endpoint;
		shiftPoint (&newendpoint, direction2);
		bool resetBacon = false;
		if (newendpoint.x == bacon.x && newendpoint.y == bacon.y) {
			growBuf (&snake2, direction2);
			resetBacon = true;
		} else {
			setGamePixel(snake2.startpoint.x, snake2.startpoint.y, 0);
			shiftBuf (&snake2, direction2);
		}
		if (getGamePixel(snake2.endpoint.x, snake2.endpoint.y))
			break;
		setGamePixel(snake2.endpoint.x, snake2.endpoint.y, 1);
		
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
		newendpoint = snake.endpoint;
		shiftPoint (&newendpoint, direction);
		resetBacon = false;
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
		p2key = BTN_NONE;
		
		for (i=0; i < ( TIME_PER_MOVE / 50 ); ++i) {
			p2key=receiveKeyPressed(50);
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

	// Prepare radio configs in global memory	
	nrf_init();
	nrf_config_set(&config);

	// Broadcast Message format: 1 Byte
	// 00 01 02 03 04 05 06 07
	// 00-07: gameID
	uint8_t gameID;
	lcdPrintln("Waiting...");
	lcdRefresh();
	if (nrf_rcv_pkt_time(timeout, 1, &gameID) != 1) // wrong package length or nothing received
		return 0;
	lcdPrintln("Found game.");
	lcdPrintln("Joining...");
	lcdRefresh();
	// At this point, we know there is an open game. Let's join it.
	
	config.mac0[4] = gameID;
	config.txmac[4] = gameID;
	nrf_config_set(&config);
	
	uint8_t init = BTN_NONE;
	delayms(20);	
	nrf_snd_pkt_crc(1, &init);
	
	return gameID; 
}

// returns 0 if timeout, gameID (!= 0) else
uint8_t switchToHostModeAndWaitForClients(int timeout) {
	
	// Broadcast Message format: 1 Byte
	// 00 01 02 03 04 05 06 07
	// 00-07: gameID
	uint8_t gameID = 0, buf;

	for (i = 0; i < timeout / 64; ++i) {
		while (!gameID)
			gameID = getRandom() & 0xffff;

		lcdPrintInt(gameID);
		lcdNl();
		lcdRefresh();
		
		config.mac0[4] = 0xff;
		config.txmac[4] = 0xff;
		nrf_config_set(&config);
		nrf_snd_pkt_crc(1, &gameID);
		
		config.mac0[4] = gameID;
		config.txmac[4] = gameID;
		nrf_config_set(&config);
		if (nrf_rcv_pkt_time(64, 1, &buf) == 1 && buf == BTN_NONE)
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
	uint8_t buf[53];
	if (nrf_rcv_pkt_time(timeout, 53, buf) != 53)
		return;
	delayms(timeout);
	memcopy(display, buf, 51);
	*baconx = buf[51];
	*bacony = buf[52];
}

// to be used by host when game display must be sent (display must be uint8_t[52])
void sendMove(uint8_t * display, uint8_t baconx, uint8_t bacony, int timeout) {
	uint8_t buf[53];
	memcopy(buf, display, 51);
	buf[51] = baconx;
	buf[52] = bacony;
	nrf_snd_pkt_crc(53, buf);
	delayms(timeout);
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
