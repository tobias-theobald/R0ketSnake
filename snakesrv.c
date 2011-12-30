#include <funk/nrf24l01p.h>
#include "snake_shared.c"

void host (void);

// radio function prototypes
void initRadio(void);
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
	//if (switchToHostModeAndWaitForClients (10000)) { // wait for 10 secs
		//someone joined
		nrf_config_set(&config);
		host();
	//}
}

inline void host (void) {
	uint8_t key = BTN_RIGHT, button;
	uint8_t p2key = BTN_LEFT;
	lcdClear();
	initSnake (&snake2, 3, GAME_HEIGHT-1, 0, GAME_HEIGHT-1, DIRECTION_LEFT);
	direction2=DIRECTION_LEFT;
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
			case BTN_NONE:
				break;
			default:
				direction2 = (direction2 + 1) % 4;
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
	delayms(500);
	while(getInputRaw() == BTN_NONE)
		delayms(25);
}

// returns 0 if timeout, gameID (!= 0) else
uint8_t switchToHostModeAndWaitForClients(int timeout) {
	
	// Broadcast Message format: 1 Byte
	// 00 01 02 03 04 05 06 07
	// 00-07: gameID
	uint8_t gameID = 0, buf;
	uint32_t k;

	for (k = 0; k < timeout / 64; ++k) {
		//while (!gameID)
		//	gameID = getRandom() & 0xff;
		gameID = 0xff;
		
		config.mac0[4] = 0xff;
		config.txmac[4] = 0xff;
		nrf_config_set(&config);
		nrf_snd_pkt_crc(1, &gameID);
		
		//config.mac0[4] = gameID;
		//config.txmac[4] = gameID;
		//nrf_config_set(&config);
		uint8_t len = nrf_rcv_pkt_time(64, 1, &buf);
		
		lcdPrintInt(len);
		lcdNl();
		lcdRefresh();		

		if (len == 1 && buf == BTN_NONE)
			return gameID;

		gameID = 0;
	}
	return 0;
}

// to be used by host in wait loop
uint8_t receiveKeyPressed(int timeout) {
	uint8_t buf;
	nrf_config_set(&config);
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
	memcpy(display, buf, 51);
	*baconx = buf[51];
	*bacony = buf[52];
}

// to be used by host when game display must be sent (display must be uint8_t[52])
void sendMove(uint8_t * display, uint8_t baconx, uint8_t bacony, int timeout) {
	uint8_t buf[53];
	memcpy(buf, display, 51);
	buf[51] = baconx;
	buf[52] = bacony;
	nrf_snd_pkt_crc(53, buf);
	delayms(timeout);
}
