#include "ESP8266RGBMatrix.h"

#define DBG_PORT Serial

#ifdef DEBUG_RGBMatrix
#define DEBUGLOG(...) DBG_PORT.printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif

///// TODO revoir les methodes avec double buffer pour une compatibilité, et passer le double buffer en variable plutot que Define
///// TODO Vérifier le _begin à chaque utilisation des buffer
///// TODO Remplacer les getFrameBufferSize par des sizeof
ESP8266RGBMatrix::ESP8266RGBMatrix() : Adafruit_GFX(RGBMATRIX_WIDTH + ADAFRUIT_GFX_EXTRA, RGBMATRIX_HEIGHT) {
	//initialisation
	_isBegin = false;
	_display_row = 0;
	_display_layer = 0;

	_framesPerSec = 1000;
	initShowTicks(); //Depend of _colorDepth and _framesPerSec

	_brightness = 255;
	setRotate(false);
	setFlip(false);
	setColorOrder(RRGGBB);
	setScanPattern(LINE);
	setBlockPattern(ABCD);
	setPanelsWidth(1);
	setColorOffset(0,0,0);

	for (uint8_t yy = 0; yy < RGBMATRIX_HEIGHT; yy++)
		_row_offset[yy] = ((yy) % RGBMATRIX_ROW_PATTERN) * SEND_BUFFER_SIZE + SEND_BUFFER_SIZE - 1;
}

void ESP8266RGBMatrix::begin() {
	if (RGBMATRIX_ROW_PATTERN == 4)
		_scan_pattern = ZIGZAG;

	init_SPI();

	pinMode(RGBMATRIX_GPIO_OE, OUTPUT);
	pinMode(RGBMATRIX_GPIO_LAT, OUTPUT);
	pinMode(RGBMATRIX_GPIO_A, OUTPUT);
	pinMode(RGBMATRIX_GPIO_B, OUTPUT);
	pinMode(RGBMATRIX_GPIO_C, OUTPUT);
	pinMode(RGBMATRIX_GPIO_D, OUTPUT);

	digitalWrite(RGBMATRIX_GPIO_OE, HIGH);
	digitalWrite(RGBMATRIX_GPIO_LAT, LOW);
	digitalWrite(RGBMATRIX_GPIO_A, LOW);
	digitalWrite(RGBMATRIX_GPIO_B, LOW);
	digitalWrite(RGBMATRIX_GPIO_C, LOW);
	digitalWrite(RGBMATRIX_GPIO_D, LOW);

	//Gestion des buffers
	_buffer = new uint8_t[BUFFER_TOTAL];
	_display_buffer = _buffer;
	_display_buffer_pos = _display_buffer;
#ifdef RGBMATRIX_DOUBLE_BUFFER
	_buffer2 = new uint8_t[BUFFER_TOTAL];
	//_edit_buffer = _buffer2;
	_active_buffer = false;
#else
	//_edit_buffer = _buffer;
#endif

	initShowTicks();

#ifdef DEBUG_RGBMatrix
	DEBUGLOG("CPU : %d MHz (CPU2X=%x)\r\n", CPU2X & 1 ? 160 : 80, CPU2X);
	DEBUGLOG("Height              %d px\r\n", RGBMATRIX_HEIGHT);
	DEBUGLOG("Width               %d px\r\n", RGBMATRIX_WIDTH);
	DEBUGLOG("Buffer total        %d bytes\r\n", BUFFER_TOTAL);
	DEBUGLOG("Row pattern         %d bits\r\n", RGBMATRIX_ROW_PATTERN);
	DEBUGLOG("Color depth         %d bits\r\n", RGBMATRIX_COLOR_DEPTH);
	DEBUGLOG("Buffer size         %d bytes\r\n", BUFFER_SIZE);
	DEBUGLOG("Pattern color bytes %d bytes\r\n", PATTERN_COLOR_BYTES);
	DEBUGLOG("Send buffer size    %d bytes\r\n", SEND_BUFFER_SIZE);
	DEBUGLOG("Max show time       %u timer_ticks\r\n", RGBMATRIX_MIN_SHOWTICKS);
	DEBUGLOG("Sequence :\r\n");
	uint32_t timePerImage = 0;
	for (uint8_t i = 0; i < _colorDepth; i++){
		uint16_t temp_showTicks = _showTicks*(1<<i);
		DEBUGLOG("%#4u (timer_ticks)  %#4u (us per row)  %#6u (cycles per row)  %#5u (us per layer) \r\n", temp_showTicks, temp_showTicks/5, temp_showTicks * 16 * (CPU2X ? 2 : 1), RGBMATRIX_ROW_PATTERN*temp_showTicks/5);
		timePerImage += RGBMATRIX_ROW_PATTERN*temp_showTicks/5;
	}
	DEBUGLOG("Temps total par image : %7.3f ms\r\n", ((float)timePerImage)/1000);
#endif

	_isBegin = true;
}

void ESP8266RGBMatrix::init_SPI() {
	uint32_t mask = ~(SPIMMOSI << SPILMOSI);
	uint16_t bits = SEND_BUFFER_SIZE * 8 - 1;
	SPI.begin();
	SPI.setFrequency(RGBMATRIX_SPI_FREQUENCY);
	SPI.setDataMode(SPI_MODE0);
	SPI.setBitOrder(MSBFIRST);
	SPI1U1 = ((SPI1U1 & mask) | (bits << SPILMOSI));
}

//TODO rename latch to layer
void ESP8266RGBMatrix::initShowTicks() {
	// 10000[ms]/ 25[img/s] / 1000[pour se mettre à la mS]
	uint32_t showticks = 5*1000*(1000/75)/RGBMATRIX_ROW_PATTERN/((1<<RGBMATRIX_COLOR_DEPTH)-1);
	if (showticks < RGBMATRIX_MIN_SHOWTICKS)
		showticks = RGBMATRIX_MIN_SHOWTICKS;
	_showTicks = showticks;
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t ESP8266RGBMatrix::color565(uint8_t r, uint8_t g, uint8_t b) {
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void ESP8266RGBMatrix::setColorOrder(color_orders color_order) {
	_color_order = color_order;
}

void ESP8266RGBMatrix::setScanPattern(scan_patterns scan_pattern) {
	_scan_pattern = scan_pattern;
}

void ESP8266RGBMatrix::setBlockPattern(block_patterns block_pattern) {
	_block_pattern = block_pattern;
}

void ESP8266RGBMatrix::setPanelsWidth(uint8_t panels) {
	_panels_width = panels;
	_panel_width_bytes = (RGBMATRIX_WIDTH / _panels_width) / 8;
}

void ESP8266RGBMatrix::setRotate(bool rotate) {
	_rotate = rotate;
}

void ESP8266RGBMatrix::setFlip(bool flip) {
	_flip = flip;
}

void ESP8266RGBMatrix::setBrightness(uint8_t brightness) {
	_brightness = brightness;
	//initShowTicks();
}

void ESP8266RGBMatrix::drawPixel(int16_t x, int16_t y, uint16_t color) {
	drawPixelRGB565(x, y, color);
}

void ESP8266RGBMatrix::showBuffer() {
#ifdef RGBMATRIX_DOUBLE_BUFFER
	_active_buffer = !_active_buffer;
	_display_buffer = _active_buffer ? _buffer2 : _buffer;
//	_edit_buffer = _active_buffer ? _buffer : _buffer2;
	_display_buffer_pos = _display_buffer + _display_layer * BUFFER_SIZE + seq_ROW_ID[_display_row];
#endif
}

void ESP8266RGBMatrix::copyBuffer(bool reverse = false) {
	// This copies the display buffer to the drawing buffer (or reverse)
	// You may need this in case you rely on the framebuffer to always contain the last frame
	// _active_buffer = true means that PxMATRIX_buffer2 is displayed
#ifdef RGBMATRIX_DOUBLE_BUFFER
	if (_active_buffer ^ reverse)
		memcpy(_buffer, _buffer2, BUFFER_TOTAL);
	else
		memcpy(_buffer2, _buffer, BUFFER_TOTAL);
#endif
}

void ESP8266RGBMatrix::setColorOffset(uint8_t r, uint8_t g, uint8_t b) {
	_color_R_offset = r;
	_color_G_offset = g;
	_color_B_offset = b;
}

void ESP8266RGBMatrix::fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b, bool selected_buffer) {
	if (r > _color_R_offset)
		r -= _color_R_offset;
	else
		r = 0;
	if (g > _color_G_offset)
		g -= _color_G_offset;
	else
		g = 0;
	if (b > _color_B_offset)
		b -= _color_B_offset;
	else
		b = 0;

	if (_block_pattern == DBCA) {
		// Every matrix is segmented in 8 blocks - 2 in X, 4 in Y direction
		// |AB|
		// |CD|
		// |AB|
		// |CD|
		// Have to rewrite this block suff and move to the scan pattern section - this will only work for chaining up to 2 panels
		if (_panels_width > 1) {  // Only works for two panels
			if ((x >= RGBMATRIX_WIDTH / 4) && (x < RGBMATRIX_WIDTH / 2))
				x += RGBMATRIX_WIDTH / 4;
			else if ((x >= RGBMATRIX_WIDTH / 2) && (x < RGBMATRIX_WIDTH * 3 / 4))
				x -= RGBMATRIX_WIDTH / 4;
		}

		uint16_t y_block = y * 4 / RGBMATRIX_HEIGHT;
		uint16_t x_block = x * 2 * _panels_width / RGBMATRIX_WIDTH;

		// Swapping A & D
		if (!(y_block % 2)) {      // Even y block
			if (!(x_block % 2)) {  // Left side of panel
				x += RGBMATRIX_WIDTH / 2 / _panels_width;
				y += RGBMATRIX_HEIGHT / 4;
			}
		} else {                // Odd y block
			if (x_block % 2) {  // Right side of panel
				x -= RGBMATRIX_WIDTH / 2 / _panels_width;
				y -= RGBMATRIX_HEIGHT / 4;
			}
		}
	}

	if (_rotate) {
		uint16_t temp_x = x;
		x = y;
		y = RGBMATRIX_HEIGHT - 1 - temp_x;
	}

	// Panels are naturally flipped
	if (!_flip)
		x = RGBMATRIX_WIDTH - 1 - x;

	if ((x < 0) || (x >= RGBMATRIX_WIDTH) || (y < 0) || (y >= RGBMATRIX_HEIGHT))
		return;

	if (_color_order != RRGGBB) {
		uint8_t r_temp = r;
		uint8_t g_temp = g;
		uint8_t b_temp = b;

		switch (_color_order) {
			case (RRGGBB):
				break;
			case (RRBBGG):
				g = b_temp;
				b = g_temp;
				break;
			case (GGRRBB):
				r = g_temp;
				g = r_temp;
				break;
			case (GGBBRR):
				r = g_temp;
				g = b_temp;
				b = r_temp;
				break;
			case (BBRRGG):
				r = b_temp;
				g = r_temp;
				b = g_temp;
				break;
			case (BBGGRR):
				r = b_temp;
				g = g_temp;
				b = r_temp;
				break;
		}
	}

	uint32_t base_offset;
	uint32_t total_offset_r = 0;
	uint32_t total_offset_g = 0;
	uint32_t total_offset_b = 0;

	if (_scan_pattern == WZAGZIG || _scan_pattern == VZAG || _scan_pattern == WZAGZIG2) {
		// get block coordinates and constraints
		uint8_t rows_per_block = ROWS_PER_BUFFER / 2;
		// this is a defining characteristic of WZAGZIG and VZAG:
		// two byte alternating chunks bottom up for WZAGZIG
		// two byte up down down up for VZAG
		uint8_t cols_per_block = 16;
		uint8_t panel_width = RGBMATRIX_WIDTH / _panels_width;
		uint8_t blocks_x_per_panel = panel_width / cols_per_block;
		uint8_t panel_index = x / panel_width;
		// strip down to single panel coordinates, restored later using panel_index
		x = x % panel_width;
		uint8_t base_y_offset = y / ROWS_PER_BUFFER;
		uint8_t buffer_y = y % ROWS_PER_BUFFER;
		uint8_t block_x = x / cols_per_block;
		uint8_t block_x_mod = x % cols_per_block;
		uint8_t block_y = buffer_y / rows_per_block;  // can only be 0/1 for height/pattern=4
		uint8_t block_y_mod = buffer_y % rows_per_block;

		// translate block address to new block address
		// invert block_y so remaining translation will be more sane
		uint8_t block_y_inv = 1 - block_y;
		uint8_t block_x_inv = blocks_x_per_panel - block_x - 1;
		uint8_t block_linear_index;
		if (_scan_pattern == WZAGZIG2) {
			block_linear_index = block_x_inv * 2 + block_y;
		} else if (_scan_pattern == WZAGZIG) {
			// apply x/y block transform for WZAGZIG, only works for height/pattern=4
			block_linear_index = block_x_inv * 2 + block_y_inv;
		} else if (_scan_pattern == VZAG) {
			// apply x/y block transform for VZAG, only works for height/pattern=4 and 32x32 panels until a larger example is found
			block_linear_index = block_x_inv * 3 * block_y + block_y_inv * (block_x_inv + 1);
		}
		// render block linear index back into normal coordinates
		uint8_t new_block_x = block_linear_index % blocks_x_per_panel;
		uint8_t new_block_y = 1 - block_linear_index / blocks_x_per_panel;
		x = new_block_x * cols_per_block + block_x_mod + panel_index * panel_width;
		y = new_block_y * rows_per_block + block_y_mod + base_y_offset * ROWS_PER_BUFFER;
	}

	// This code sections computes the byte in the buffer that will be manipulated.
	if (_scan_pattern != LINE && _scan_pattern != WZAGZIG && _scan_pattern != VZAG && _scan_pattern != WZAGZIG2) {
		// Precomputed row offset values
		base_offset = _row_offset[y] - (x / 8) * 2;
		uint8_t row_sector = 0;
		uint16_t row_sector__offset = RGBMATRIX_WIDTH / 4;
		for (uint8_t yy = 0; yy < RGBMATRIX_HEIGHT; yy += 2 * RGBMATRIX_ROW_PATTERN) {
			if ((yy <= y) && (y < yy + RGBMATRIX_ROW_PATTERN))
				total_offset_r = base_offset - row_sector__offset * row_sector;
			if ((yy + RGBMATRIX_ROW_PATTERN <= y) && (y < yy + 2 * RGBMATRIX_ROW_PATTERN))
				total_offset_r = base_offset - row_sector__offset * row_sector;
			row_sector++;
		}
	} else {
		// can only be non-zero when _height/(2 inputs per panel)/_row_pattern > 1
		// i.e.: 32x32 panel with 1/8 scan (A/B/C lines) -> 32/2/8 = 2
		uint8_t vert_index_in_buffer = (y % ROWS_PER_BUFFER) / RGBMATRIX_ROW_PATTERN;  // which set of rows per buffer

		// can only ever be 0/1 since there are only ever 2 separate input sets present for this variety of panels (R1G1B1/R2G2B2)
		uint8_t which_buffer = y / ROWS_PER_BUFFER;
		uint8_t x_byte = x / 8;
		// assumes panels are only ever chained for more width
		uint8_t which_panel = x_byte / _panel_width_bytes;
		uint8_t in_row_byte_offset = x_byte % _panel_width_bytes;
		// this could be pretty easily extended to vertical stacking as well
		total_offset_r = _row_offset[y] - in_row_byte_offset - _panel_width_bytes * (ROW_SETS_PER_BUFFER * (_panels_width * which_buffer + which_panel) + vert_index_in_buffer);
	}

	uint8_t bit_select = x % 8;

	// Normally the bytes in one buffer would be sequencial, e.g.
	// 0-1-2-3-
	// 0-1-2-3-
	// hence the upper and lower row start with [OL|OH].
	//
	// However some panels have a byte wise row-changing scanning pattern and/or a bit changing pattern that we have to take case of
	// For example  [1L|1H] [3L|3H] for ZIGZAG or [0L|0H] [2L|2H] for ZAGZIG
	//                 |   \   |   \                 |   /   |   /
	//              [0L|0H] [2L|2H]               [1L|1H] [3L|3H]
	//
	// For example  [0H|1L] [2H|3L] for ZZAGG  or [0L|1L] [2L|3L] for ZZIAGG
	//                 |   \   |   \                 |   /   |   /
	//              [0L|1H] [2L|3H]               [0H|1H] [2H|3H]
	//
	//
	// In order to make the pattern start on both rows with [0L|0H] we have to add / subtract values to / from total_offset_r and bit_select
	if ((y % (RGBMATRIX_ROW_PATTERN * 2)) < RGBMATRIX_ROW_PATTERN) {
		// Variant of ZAGZIG pattern with bit oder reversed on lower part (starts on upper part)
		if (_scan_pattern == ZAGGIZ) {
			total_offset_r--;
			bit_select = 7 - bit_select;
		}

		// Row changing pattern (starts on upper part)
		if (_scan_pattern == ZAGZIG)
			total_offset_r--;

		// Byte split pattern - like ZAGZIG but after every 4 bit (starts on upper part)
		if (_scan_pattern == ZZIAGG) {
			if (bit_select > 3)
				bit_select += 4;
			else
				total_offset_r--;
		}

		// Byte split pattern (lower part)
		if (_scan_pattern == ZZAGG)
			if (bit_select > 3)
				total_offset_r--;
	} else {
		if (_scan_pattern == ZIGZAG)
			total_offset_r--;

		// Byte split pattern - like ZAGZIG but after every 4 bit (starts on upper part)
		if (_scan_pattern == ZZIAGG) {
			if (bit_select > 3) {
				total_offset_r--;
				bit_select -= 4;
			}
		}

		// Byte split pattern (upper part)
		if (_scan_pattern == ZZAGG) {
			if (bit_select <= 3)
				bit_select += 4;
			else {
				bit_select -= 4;
				total_offset_r--;
			}
		}
	}

	total_offset_g = total_offset_r - PATTERN_COLOR_BYTES;
	total_offset_b = total_offset_g - PATTERN_COLOR_BYTES;

#ifdef RGBMATRIX_DOUBLE_BUFFER
	uint8_t* temp_bufferp = selected_buffer ? _buffer2 : _buffer;
#else
	uint8_t* temp_bufferp = _buffer;
#endif

	r = r >> (8 - RGBMATRIX_COLOR_DEPTH);
	g = g >> (8 - RGBMATRIX_COLOR_DEPTH);
	b = b >> (8 - RGBMATRIX_COLOR_DEPTH);

	//Color interlacing
	for (int this_color_bit = 0; this_color_bit < RGBMATRIX_COLOR_DEPTH; this_color_bit++) {
		if ((r >> this_color_bit) & 0x01)
			temp_bufferp[this_color_bit * BUFFER_SIZE + total_offset_r] |= _BV(bit_select);
		else
			temp_bufferp[this_color_bit * BUFFER_SIZE + total_offset_r] &= ~_BV(bit_select);

		if ((g >> this_color_bit) & 0x01)
			temp_bufferp[this_color_bit * BUFFER_SIZE + total_offset_g] |= _BV(bit_select);
		else
			temp_bufferp[this_color_bit * BUFFER_SIZE + total_offset_g] &= ~_BV(bit_select);

		if ((b >> this_color_bit) & 0x01)
			temp_bufferp[this_color_bit * BUFFER_SIZE + total_offset_b] |= _BV(bit_select);
		else
			temp_bufferp[this_color_bit * BUFFER_SIZE + total_offset_b] &= ~_BV(bit_select);
	}
}

void ESP8266RGBMatrix::drawPixelRGB565(int16_t x, int16_t y, uint16_t color) {
	uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
	uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
	uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;
#ifdef RGBMATRIX_DOUBLE_BUFFER
	fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
	fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

void ESP8266RGBMatrix::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
#ifdef RGBMATRIX_DOUBLE_BUFFER
	fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
	fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

uint8_t ESP8266RGBMatrix::getPixel(int8_t x, int8_t y) {
	return (0);  //PxMATRIX_buffer[x+ (y/8)*LCDWIDTH] >> (y%8)) & 0x1;
}

bool ESP8266RGBMatrix::enable() {
	if (!_isBegin){
		DEBUGLOG("Must call begin() before enable()");
		return false;
	}
	// Premier appel pour initiliser les pointeurs
	// Timer1 automaticly adjuste ticks
	refresh();
	timer1_isr_init();
	timer1_attachInterrupt(ESP8266RGBMatrix::refreshCallback);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);	 //TIM_DIV16 5MHz (5 ticks/us - 1677721.4 us max)
	timer1_write(50);  // 50 ticks = 5*10 us = 50us
	return true;
}

void ESP8266RGBMatrix::disable() {
	timer1_disable();
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << RGBMATRIX_GPIO_OE);	//Force panel off
}

void ESP8266RGBMatrix::refreshCallback() {
	display.refresh();
}

void ESP8266RGBMatrix::refreshTest(){
	if (!_isBegin){
		DEBUGLOG("Must call begin() before enable()");
		return;
	}
	uint32_t sumV = 0;
	uint16_t minV = 0xFFFF;
	uint16_t maxV = 0;
	for(int i = 0; i<RGBMATRIX_ROW_PATTERN*RGBMATRIX_COLOR_DEPTH; i++){
		uint32_t deb = asm_ccount();
		refresh();
		uint32_t delta = asm_ccount() - deb;
		sumV+=delta;
		if (delta==minV)
			DEBUGLOG(".");
		else
			DEBUGLOG("[%d]",delta);		
		if (delta<minV)	minV = delta;
		if (delta>maxV)	maxV = delta;
	}
	DEBUGLOG("\r\nMin=%d\tMax=%d\tSum=%d\r\n",minV,maxV,sumV);
}

inline void ESP8266RGBMatrix::refresh() {
	//Min=214	Max=226	Sum=20928 (avant passage _color_depth en variable)
	//Min=215	Max=227	Sum=20992 (avant calcul de durée plutot que tableau
	//Min=215	Max=227	Sum=20995
	noInterrupts();
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (1 << RGBMATRIX_GPIO_OE));
	if (_display_layer == 0)
		GPIO_REG_WRITE(seq_REG_CMD[_display_row], seq_REG_VAL[_display_row]);
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (1 << RGBMATRIX_GPIO_LAT));
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, (1 << RGBMATRIX_GPIO_LAT)+(1 << RGBMATRIX_GPIO_OE));

	_display_layer++;
	if (_display_layer == RGBMATRIX_COLOR_DEPTH) {
		_display_layer = 0;
		_display_row = (_display_row + 1) & (RGBMATRIX_ROW_PATTERN-1);
		_display_buffer_pos = _display_buffer + seq_ROW_ID[_display_row];
	}
	else
		_display_buffer_pos += BUFFER_SIZE;

	memcpy((void *)&SPI1W0, _display_buffer_pos , SEND_BUFFER_SIZE);
	SPI1CMD |= SPIBUSY;

	T1L = _showTicks*(1<<_display_layer);
	interrupts();
}

///// TODO revoir les methodes avec double buffer pour une compatibilité, et passer le double buffer en variable plutot que Define
///// TODO passer RGBMATRIX_SHOWTIME en normal, plus en DEFINE

// clear everything
void ESP8266RGBMatrix::clearDisplay() {
#ifdef RGBMATRIX_DOUBLE_BUFFER
	clearDisplay(!_active_buffer);
#else
	memset(_buffer, 0, BUFFER_TOTAL);
#endif
}

#ifdef RGBMATRIX_DOUBLE_BUFFER
void ESP8266RGBMatrix::clearDisplay(bool selected_buffer) {
	memset(selected_buffer ? _buffer2 : _buffer, 0, BUFFER_TOTAL);
}
#endif

ESP8266RGBMatrix display;
