#define SCREEN_WIDTH  96
#define SCREEN_HEIGHT 67
#define SCREEN_SIZE ( SCREEN_WIDTH * SCREEN_HEIGHT )

#define INITIAL_LENGTH 4

typedef struct {
	int8_t x;
	int8_t y;
} point;

typedef struct {
	int8_t start;
	int8_t length;
	point[SCREEN_SIZE] data; // TODO: optimize to a power of 2
} vringpbuf; // variable (but limited) size ring buffer for points

vringpbuf snake;

int8_t i;

void initSnake (vringpbuf *);

void game (void) {
	lcdClear();
	
	while (1) {
		
		
	}
}

void initSnake (vringpbuf *s) {
	s->start=0;
	s->length=INITIAL_LENGTH;
	for (i=0; i<INITIAL_LENGTH; i++) {
		s->data[i].x=i;
		s->data[i].y=0;
	}
}

