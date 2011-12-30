#include "snake_shared.c"

void ram (void) {
	uint8_t key = BTN_RIGHT, button;
	lcdClear();
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
