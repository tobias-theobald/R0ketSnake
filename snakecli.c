#include "funk/nrf24l01p.h"
#include "snake_shared.c"

void client (void);

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
	lcdClear();
	lcdPrintln(" looking for ");
	lcdPrintln("  existing   ");
	lcdPrintln("   games...  ");
	lcdRefresh();
	if (initRadioAndLookForGames(2000)) { // game found
		// act as controller and screen
		lcdClear();
		lcdRefresh();
		client();
	} else { // no game found
		lcdClear();
		lcdPrintln("No game found");
		lcdRefresh();
		delayms(500);
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

	// Prepare radio configs in global memory
	nrf_config_set(&config);

	// Broadcast Message format: 1 Byte
	// 00 01 02 03 04 05 06 07
	// 00-07: gameID
	uint8_t gameID;
	lcdPrintln("Waiting...");
	lcdRefresh();
	uint32_t len = nrf_rcv_pkt_time(timeout, 1, &gameID);
	lcdPrintInt(len);
	lcdNl();
	lcdRefresh();
	if (len != 1) // wrong package length or nothing received
		return 0;
	lcdPrintln("Found game.");
	lcdPrintln("Joining...");
	lcdRefresh();
	// At this point, we know there is an open game. Let's join it.
	
	//config.mac0[4] = gameID;
	//config.txmac[4] = gameID;
	//nrf_config_set(&config);
	
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
