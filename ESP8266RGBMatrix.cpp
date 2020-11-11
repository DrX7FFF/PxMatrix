#include "ESP8266RGBMatrix.h"

#define DBG_PORT Serial

#ifdef DEBUG_RGBMatrix
#define DEBUGLOG(...) DBG_PORT.printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif

///// TODO Vérifier le _begin à chaque utilisation des buffer
ESP8266RGBMatrix::ESP8266RGBMatrix() {
	//initialisation
	_isBegin = false;
	_display_row = 0;
	_display_layer = 0;

	//default values
	_colorDepth = RGBMATRIX_DEFAULT_COLOR_DEPTH;
	_framesPerSec = 1000;

	_brightness = 255;
	_rotate = false;
	_flip = false;
	_color_order = RRGGBB;
	_scan_pattern = LINE;
	_block_pattern = ABCD;
	_panels_width = 1;
	setColorOffset(0,0,0);
}

void ESP8266RGBMatrix::setGPIO(uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C) {
	initGPIO(3, gpio_OE, gpio_LAT, gpio_A, gpio_B, gpio_C, 0xFF, 0xFF);
}

void ESP8266RGBMatrix::setGPIO(uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C, uint8_t gpio_D) {
	initGPIO(4, gpio_OE, gpio_LAT, gpio_A, gpio_B, gpio_C, gpio_D, 0xFF);
}

void ESP8266RGBMatrix::setGPIO(uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C, uint8_t gpio_D, uint8_t gpio_E) {
	initGPIO(5, gpio_OE, gpio_LAT, gpio_A, gpio_B, gpio_C, gpio_D, gpio_E);
}

void ESP8266RGBMatrix::initGPIO(uint8_t muxBits, uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C, uint8_t gpio_D, uint8_t gpio_E) {
	_muxBits = muxBits;
	_rowPattern = 1<<_muxBits;

	SPI.begin();
	SPI.setFrequency(RGBMATRIX_SPI_FREQUENCY);
	SPI.setDataMode(SPI_MODE0);
	SPI.setBitOrder(MSBFIRST);

	pinMode(gpio_OE, OUTPUT);
	digitalWrite(gpio_OE, HIGH);
	_mask_OE = 1<<gpio_OE;
	pinMode(gpio_LAT, OUTPUT);
	digitalWrite(gpio_LAT, LOW);
	_mask_LAT = 1<<gpio_LAT;
	pinMode(gpio_A, OUTPUT);
	digitalWrite(gpio_A, LOW);
	_mask_A = 1<<gpio_A;
	pinMode(gpio_B, OUTPUT);
	digitalWrite(gpio_B, LOW);
	_mask_B = 1<<gpio_B;
	pinMode(gpio_C, OUTPUT);
	digitalWrite(gpio_C, LOW);
	_mask_C = 1<<gpio_C;
	if (_muxBits>=4){
		pinMode(gpio_D, OUTPUT);
		digitalWrite(gpio_D, LOW);
		_mask_D = 1<<gpio_D;
	}
	if (_muxBits>=5){
		pinMode(gpio_E, OUTPUT);
		digitalWrite(gpio_E, LOW);
		_mask_E = 1<<gpio_E;
	}
}

void ESP8266RGBMatrix::begin(uint16_t width, uint16_t height, uint8_t colorDepth) {
	begin(width, height, colorDepth, false);
}

void ESP8266RGBMatrix::begin(uint16_t width, uint16_t height, uint8_t colorDepth, bool doubleBuffer) {
	_width = width;
	_height = height;
	_doubleBuffer = doubleBuffer;
	if (colorDepth<1)			_colorDepth = 1;
	else if (colorDepth>8)		_colorDepth = 8;
	else						_colorDepth = colorDepth;

	_bufferSize = (_height * _width * 3 / 8);
	_patternColorBytes = (_height / _rowPattern) * (_width / 8);
	_sendBufferSize = _patternColorBytes * 3;

	if (_rowPattern == 4)
		_scan_pattern = ZIGZAG;

	//Gestion des buffers
	_buffer = new uint8_t[_colorDepth * _bufferSize];
	_display_buffer = _buffer;
	_display_buffer_pos = _display_buffer;
	_active_buffer = false;
	if (_doubleBuffer){
		_buffer2 = new uint8_t[_colorDepth * _bufferSize];
		_edit_buffer = _buffer2;
	}
	else
		_edit_buffer = _buffer;

	initShowTicks();
	initPatternSeq();
	initPreIndex();
	init_SPIBufferSize();
	
#ifdef DEBUG_RGBMatrix
	DEBUGLOG("CPU : %d MHz (CPU2X=%x)\r\n", CPU2X & 1 ? 160 : 80, CPU2X);
	DEBUGLOG("Width               %#4u px\r\n", _width);
	DEBUGLOG("Height              %#4u px\r\n", _height);
	DEBUGLOG("Frame Buffer        %#4u bytes\r\n", _colorDepth * _bufferSize);
	DEBUGLOG("Mux length          %#4u bits\r\n", _muxBits);
	DEBUGLOG("Row pattern         %#4u\r\n", _rowPattern);
	DEBUGLOG("Color depth         %#4u bits\r\n", _colorDepth);
	DEBUGLOG("Buffer size         %#4u bytes\r\n", _bufferSize);
	DEBUGLOG("Pattern color bytes %#4u bytes\r\n", _patternColorBytes);
	DEBUGLOG("Send buffer size    %#4u bytes\r\n", _sendBufferSize);
	DEBUGLOG("OE Mask                 %X \r\n", _mask_OE);
	DEBUGLOG("LAT Mask                %X \r\n", _mask_LAT);
	DEBUGLOG("A Mask                  %X\r\n", _mask_A);
	DEBUGLOG("B Mask                  %X\r\n", _mask_B);
	DEBUGLOG("C Mask                  %X\r\n", _mask_C);
	DEBUGLOG("D Mask                  %X\r\n", _mask_D);
	DEBUGLOG("E Mask                  %X\r\n", _mask_E);
	
	DEBUGLOG("Layer sequence :\r\n");
	uint32_t timePerImage = 0;
	for (uint8_t i = 0; i < _colorDepth; i++){
		uint16_t temp_showTicks = _showTicks*(1<<i);
		DEBUGLOG("%#4u (timer_ticks)  %#4u (us per row)  %#6u (cycles per row)  %#5u (us per layer) \r\n", temp_showTicks, temp_showTicks/5, temp_showTicks * 16 * (CPU2X ? 2 : 1), _rowPattern*temp_showTicks/5);
		timePerImage += _rowPattern*temp_showTicks/5;
	}
	DEBUGLOG("Total time per image : %7.3f ms\r\n", ((float)timePerImage)/1000);
#endif
	_isBegin = true;
}

void ESP8266RGBMatrix::init_SPIBufferSize() {
	uint32_t mask = ~(SPIMMOSI << SPILMOSI);
	uint16_t bits = _sendBufferSize * 8 - 1;
	SPI1U1 = ((SPI1U1 & mask) | (bits << SPILMOSI));
}

void ESP8266RGBMatrix::initShowTicks() {
	// 10000[ms]/ 25[img/s] / 1000[pour se mettre à la mS]
	uint32_t showticks = 5*1000*(1000/_framesPerSec)/_rowPattern/((1<<_colorDepth)-1);

	// 5[coefTimer1] * 1 000 000 [en ms] * _sendBufferSize*8 [Bits send] / RGBMATRIX_SPI_FREQUENCY [SPI Debit] 
	//entre 50 et 25K ticks 
	uint32_t minShowTicks = 5*1000000*_sendBufferSize*8/RGBMATRIX_SPI_FREQUENCY;
	DEBUGLOG("Minimum ShowTicks = %u at SPI = %u Hz",  minShowTicks, RGBMATRIX_SPI_FREQUENCY);
	if (showticks < minShowTicks)
		showticks = minShowTicks;
	_showTicks = showticks;
}

void ESP8266RGBMatrix::initPatternSeq(){
	if (_muxSeq)
		delete _muxSeq;
	_muxSeq = new muxStruct[_rowPattern];
	DEBUGLOG("Row pattern sequence :\r\n");
	uint8_t prevIndex = (_rowPattern-1) ^ ((_rowPattern-1)>>1);
	for(int i=0; i<_rowPattern; i++){
		uint8_t newIndex = i ^ (i>>1);
		_muxSeq[i].cmd 		= newIndex>prevIndex?GPIO_OUT_W1TS_ADDRESS:GPIO_OUT_W1TC_ADDRESS;
		_muxSeq[i].offset	= newIndex*_sendBufferSize;
		switch (prevIndex^newIndex){
		case 0b00001:
			_muxSeq[i].val =_mask_A;
			break;
		case 0b00010:
			_muxSeq[i].val =_mask_B;
			break;
		case 0b00100:
			_muxSeq[i].val =_mask_C;
			break;
		case 0b01000:
			_muxSeq[i].val =_mask_D;
			break;
		case 0b10000:
			_muxSeq[i].val =_mask_E;
			break;
		}
		DEBUGLOG("%#2u| - %04u(%02u) vers %04u(%02u) cmd:%u %08u offset %#4u\r\n", i, D2B(prevIndex),prevIndex,D2B(newIndex),newIndex, _muxSeq[i].cmd, D2B(_muxSeq[i].val), _muxSeq[i].offset);
		prevIndex = newIndex;
	}
}

void ESP8266RGBMatrix::initPreIndex(){
	if (_row_offset)
		delete _row_offset;
	_row_offset = new uint32_t[_height];
 	for (uint8_t yy = 0; yy < _height; yy++)
		_row_offset[yy] = ((yy) % _rowPattern) * _sendBufferSize + _sendBufferSize - 1;
}

void ESP8266RGBMatrix::setBrightness(uint8_t brightness) {
	_brightness = brightness;
	//initShowTicks();
}

void ESP8266RGBMatrix::showBuffer() {
	if (_doubleBuffer){
		_active_buffer = !_active_buffer;
		_display_buffer = _active_buffer ? _buffer2 : _buffer;
		_display_buffer_pos = _display_buffer + _display_layer * _bufferSize + _muxSeq[_display_row].offset;
		_edit_buffer = _active_buffer ? _buffer : _buffer2;
	}
}

void ESP8266RGBMatrix::clearDisplay() {
	clearDisplay(!_active_buffer);
}

void ESP8266RGBMatrix::clearDisplay(bool selected_buffer) {
	if (_doubleBuffer)
		memset(selected_buffer ? _buffer2 : _buffer, 0, sizeof(_buffer));
	else
		memset(_buffer, 0, sizeof(_buffer));
}

void ESP8266RGBMatrix::copyBuffer(bool reverse = false) {
	// This copies the display buffer to the drawing buffer (or reverse)
	// You may need this in case you rely on the framebuffer to always contain the last frame
	// _active_buffer = true means that PxMATRIX_buffer2 is displayed
	if (_doubleBuffer){
		if (_active_buffer ^ reverse)
			memcpy(_buffer, _buffer2, sizeof(_buffer));
		else
			memcpy(_buffer2, _buffer, sizeof(_buffer2));
	}
}

void ESP8266RGBMatrix::setColorOffset(uint8_t r, uint8_t g, uint8_t b) {
	_color_R_offset = r;
	_color_G_offset = g;
	_color_B_offset = b;
}

void ESP8266RGBMatrix::setPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
	uint8_t rows_per_buffer = (_height / 2);

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
			if ((x >= _width / 4) && (x < _width / 2))
				x += _width / 4;
			else if ((x >= _width / 2) && (x < _width * 3 / 4))
				x -= _width / 4;
		}

		uint16_t y_block = y * 4 / _height;
		uint16_t x_block = x * 2 * _panels_width / _width;

		// Swapping A & D
		if (!(y_block % 2)) {      // Even y block
			if (!(x_block % 2)) {  // Left side of panel
				x += _width / 2 / _panels_width;
				y += _height / 4;
			}
		} else {                // Odd y block
			if (x_block % 2) {  // Right side of panel
				x -= _width / 2 / _panels_width;
				y -= _height / 4;
			}
		}
	}

	if (_rotate) {
		uint16_t temp_x = x;
		x = y;
		y = _height - 1 - temp_x;
	}

	// Panels are naturally flipped
	if (!_flip)
		x = _width - 1 - x;

	if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
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
		uint8_t rows_per_block = rows_per_buffer / 2;
		// this is a defining characteristic of WZAGZIG and VZAG:
		// two byte alternating chunks bottom up for WZAGZIG
		// two byte up down down up for VZAG
		uint8_t cols_per_block = 16;
		uint8_t panel_width = _width / _panels_width;
		uint8_t blocks_x_per_panel = panel_width / cols_per_block;
		uint8_t panel_index = x / panel_width;
		// strip down to single panel coordinates, restored later using panel_index
		x = x % panel_width;
		uint8_t base_y_offset = y / rows_per_buffer;
		uint8_t buffer_y = y % rows_per_buffer;
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
		y = new_block_y * rows_per_block + block_y_mod + base_y_offset * rows_per_buffer;
	}

	// This code sections computes the byte in the buffer that will be manipulated.
	if (_scan_pattern != LINE && _scan_pattern != WZAGZIG && _scan_pattern != VZAG && _scan_pattern != WZAGZIG2) {
		// Precomputed row offset values
		base_offset = _row_offset[y] - (x / 8) * 2;
		uint8_t row_sector = 0;
		uint16_t row_sector__offset = _width / 4;
		for (uint8_t yy = 0; yy < _height; yy += 2 * _rowPattern) {
			if ((yy <= y) && (y < yy + _rowPattern))
				total_offset_r = base_offset - row_sector__offset * row_sector;
			if ((yy + _rowPattern <= y) && (y < yy + 2 * _rowPattern))
				total_offset_r = base_offset - row_sector__offset * row_sector;
			row_sector++;
		}
	} else {
		uint8_t	_panel_width_bytes = (_width / _panels_width) / 8;
		// can only be non-zero when _height/(2 inputs per panel)/_row_pattern > 1
		// i.e.: 32x32 panel with 1/8 scan (A/B/C lines) -> 32/2/8 = 2
		uint8_t vert_index_in_buffer = (y % rows_per_buffer) / _rowPattern;  // which set of rows per buffer

		// can only ever be 0/1 since there are only ever 2 separate input sets present for this variety of panels (R1G1B1/R2G2B2)
		uint8_t which_buffer = y / rows_per_buffer;
		uint8_t x_byte = x / 8;
		// assumes panels are only ever chained for more width
		uint8_t which_panel = x_byte / _panel_width_bytes;
		uint8_t in_row_byte_offset = x_byte % _panel_width_bytes;
		// this could be pretty easily extended to vertical stacking as well
		total_offset_r = _row_offset[y] - in_row_byte_offset - _panel_width_bytes * ((rows_per_buffer / _rowPattern) * (_panels_width * which_buffer + which_panel) + vert_index_in_buffer);
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
	if ((y % (_rowPattern * 2)) < _rowPattern) {
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

	total_offset_g = total_offset_r - _patternColorBytes;
	total_offset_b = total_offset_g - _patternColorBytes;

	r = r >> (8 - _colorDepth);
	g = g >> (8 - _colorDepth);
	b = b >> (8 - _colorDepth);

	//Color interlacing
	for (int this_color_bit = 0; this_color_bit < _colorDepth; this_color_bit++) {
		if ((r >> this_color_bit) & 0x01)
			_edit_buffer[this_color_bit * _bufferSize + total_offset_r] |= _BV(bit_select);
		else
			_edit_buffer[this_color_bit * _bufferSize + total_offset_r] &= ~_BV(bit_select);

		if ((g >> this_color_bit) & 0x01)
			_edit_buffer[this_color_bit * _bufferSize + total_offset_g] |= _BV(bit_select);
		else
			_edit_buffer[this_color_bit * _bufferSize + total_offset_g] &= ~_BV(bit_select);

		if ((b >> this_color_bit) & 0x01)
			_edit_buffer[this_color_bit * _bufferSize + total_offset_b] |= _BV(bit_select);
		else
			_edit_buffer[this_color_bit * _bufferSize + total_offset_b] &= ~_BV(bit_select);
	}
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
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, _mask_OE);	//Force panel off
}

void ESP8266RGBMatrix::refreshCallback() {
	RGBMatrix.refresh();
}

void ESP8266RGBMatrix::refreshTest(){
	if (!_isBegin){
		DEBUGLOG("Must call begin() before enable()");
		return;
	}
	uint32_t sumV = 0;
	uint16_t minV = 0xFFFF;
	uint16_t maxV = 0;
	for(int i = 0; i<_rowPattern*_colorDepth; i++){
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
	//Min=200	Max=215	Sum=19612

	noInterrupts();
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, _mask_OE);
	if (_display_layer == 0)
		GPIO_REG_WRITE(_muxSeq[_display_row].cmd, _muxSeq[_display_row].val);
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, _mask_LAT);
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, _mask_LAT + _mask_OE);

	_display_layer++;
	if (_display_layer == _colorDepth) {
		_display_layer = 0;
		_display_row = (_display_row + 1) & (_rowPattern-1);
		_display_buffer_pos = _display_buffer + _muxSeq[_display_row].offset;
	}
	else
		_display_buffer_pos += _bufferSize;

	memcpy((void *)&SPI1W0, _display_buffer_pos , _sendBufferSize);
	SPI1CMD |= SPIBUSY;

	T1L = _showTicks*(1<<_display_layer);
	interrupts();
}

ESP8266RGBMatrix RGBMatrix;
