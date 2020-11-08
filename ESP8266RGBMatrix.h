#ifndef ESP8266RGBMatrix_H
#define ESP8266RGBMatrix_H

#define DEBUG_RGBMatrix  //Uncomment this to enable debug messages over serial port

// Pins affectation
#ifndef RGBMATRIX_GPIO_A
#define RGBMATRIX_GPIO_A 0
#endif
#ifndef RGBMATRIX_GPIO_B
#define RGBMATRIX_GPIO_B 4
#endif
#ifndef RGBMATRIX_GPIO_C
#define RGBMATRIX_GPIO_C 5
#endif
#ifndef RGBMATRIX_GPIO_D
#define RGBMATRIX_GPIO_D 2
#endif
#ifndef RGBMATRIX_GPIO_OE
#define RGBMATRIX_GPIO_OE 15
#endif
#ifndef RGBMATRIX_GPIO_LAT
#define RGBMATRIX_GPIO_LAT 12
#endif

// Matrix pixels width
#ifndef RGBMATRIX_WIDTH
#define RGBMATRIX_WIDTH 32
#endif
// Matrix pixels height
#ifndef RGBMATRIX_HEIGHT
#define RGBMATRIX_HEIGHT 32
#endif

#ifndef RGBMATRIX_ROW_PATTERN
#define RGBMATRIX_ROW_PATTERN 16
#endif
#ifndef RGBMATRIX_COLOR_DEPTH
#define RGBMATRIX_COLOR_DEPTH 6
#endif

//#define RGBMATRIX_DOUBLE_BUFFER true

// Technique param 
//30 avec 6bits = 66 img/s
//25 avec 6bits = 79 img/s
//25 avec 8bits = 20 img/s
//15 avec 8bits = 32 img/s mais sintillement
#define RGBMATRIX_SHOWTIME 50 //entre 50 et 25K

// Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
#ifndef RGBMATRIX_SPI_FREQUENCY
#define RGBMATRIX_SPI_FREQUENCY 20000000
#endif

// Helper
#define BUFFER_TOTAL (RGBMATRIX_COLOR_DEPTH * BUFFER_SIZE)
#define BUFFER_SIZE (RGBMATRIX_HEIGHT * RGBMATRIX_WIDTH * 3 / 8)
#define PATTERN_COLOR_BYTES ((RGBMATRIX_HEIGHT / RGBMATRIX_ROW_PATTERN) * (RGBMATRIX_WIDTH / 8))
#define SEND_BUFFER_SIZE (PATTERN_COLOR_BYTES * 3)
#define ROWS_PER_BUFFER (RGBMATRIX_HEIGHT / 2)
#define ROW_SETS_PER_BUFFER	(ROWS_PER_BUFFER / RGBMATRIX_ROW_PATTERN)

#ifndef _BV
#define _BV(x) (1 << (x))
#endif
#define ROW2PIN(val) ((val & 1 ? 1 << RGBMATRIX_GPIO_A : 0) + (val & 2 ? 1 << RGBMATRIX_GPIO_B : 0) + (val & 4 ? 1 << RGBMATRIX_GPIO_C : 0) + (val & 8 ? 1 << RGBMATRIX_GPIO_D : 0))

#include <SPI.h>
#include "Adafruit_GFX.h"
//#include "Arduino.h"

/* asm-helpers */
static inline int32_t asm_ccount(void) {
    int32_t r; asm volatile ("rsr %0, ccount" : "=r"(r)); return r; }

// Sometimes some extra width needs to be passed to Adafruit GFX constructor
// to render text close to the end of the display correctly
#ifndef ADAFRUIT_GFX_EXTRA
#define ADAFRUIT_GFX_EXTRA 0
#endif

// Specifies what blocking pattern the panel is using
// |AB|,|DB|
// |CD|,|CA|
// |AB|,|DB|
// |CD|,|CA|
enum block_patterns { ABCD,
					  DBCA };

// This is how the scanning is implemented. LINE just scans it left to right,
// ZIGZAG jumps 4 rows after every byte, ZAGGII alse revereses every second byte
enum scan_patterns { LINE,
					 ZIGZAG,
					 ZZAGG,
					 ZAGGIZ,
					 WZAGZIG,
					 VZAG,
					 ZAGZIG,
					 WZAGZIG2,
					 ZZIAGG };

// Specify the color order
enum color_orders { RRGGBB,
					RRBBGG,
					GGRRBB,
					GGBBRR,
					BBRRGG,
					BBGGRR };

class ESP8266RGBMatrix : public Adafruit_GFX {
public:
	uint8_t getRow() {return _display_row;};
	uint8_t getColor() {return _display_color;};
	uint16_t getLayerTime() {return _latch_seq[_display_color];};

	ESP8266RGBMatrix();
	void begin();
	void disable();
	void enable();
	void enableCallibration();
	void setMSCalibration(uint32_t speed);
	static inline void refreshCallback();
	static inline void refreshCalibrationCallback();
	inline void refresh();
	inline void refreshCalibration();
	void refreshTest();
	void dumpImgCount();
	void clearDisplay();
	void clearDisplay(bool selected_buffer);
	//inline uint8_t* getEditBuffer() { return _edit_buffer; };
	void drawPixelRGB565(int16_t x, int16_t y, uint16_t color);	// Draw pixels
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
	uint8_t getPixel(int8_t x, int8_t y);                // Does nothing for now (always returns 0)
	uint16_t color565(uint8_t r, uint8_t g, uint8_t b);  // Converts RGB888 to RGB565
	void showBuffer();
#ifdef RGBMATRIX_DOUBLE_BUFFER
	void copyBuffer(bool reverse);
#endif

	void setBrightness(uint8_t brightness);			// Set the brightness of the panels (default is 255)
	void setRotate(bool rotate);  					// Rotate display
	void setFlip(bool flip);      					// Flip display
	void setColorOrder(color_orders color_order);	// Set the color order
	void setScanPattern(scan_patterns scan_pattern);	// Set the multiplex pattern {LINE, ZIGZAG, ZAGGIZ, WZAGZIG, VZAG, WZAGZIG2} (default is LINE)
	void setBlockPattern(block_patterns block_pattern);	// Set the block pattern {ABCD, DBCA} (default is ABCD)
	void setColorOffset(uint8_t r, uint8_t g, uint8_t b);// Control the minimum color values that result in an active pixel
	void setPanelsWidth(uint8_t panels);			// Set the number of panels that make up the display area width (default is 1)

private:
	uint32_t _imgCount;
	uint8_t _brightness;
	bool _rotate;
	bool _flip;
	color_orders _color_order;		// Holds the color order
	scan_patterns _scan_pattern;    // Holds the scan pattern
	block_patterns _block_pattern;  // Holds the block pattern
	uint8_t _color_R_offset = 0;	// Color offset
	uint8_t _color_G_offset = 0;
	uint8_t _color_B_offset = 0;
	uint8_t _panels_width;

	uint8_t _panel_width_bytes;
	bool _isBegin;
	uint8_t _display_row;
	uint8_t _display_color;

	// Holds some pre-computed values for faster pixel drawing
	uint16_t _latch_seq[RGBMATRIX_COLOR_DEPTH];
	uint32_t _row_offset[RGBMATRIX_HEIGHT];

	//Gestion des buffers
	uint8_t* _buffer;
	uint8_t* _display_buffer;
	uint8_t* _display_buffer_pos;
	uint8_t* _display_buffer_pos2;
#ifdef RGBMATRIX_DOUBLE_BUFFER
	uint8_t* _buffer2;
	//uint8_t* _edit_buffer;
	bool _active_buffer;
#else
	//uint8_t* _edit_buffer = _buffer;
#endif

	void init_SPI();
	void initLatchSeq();
	// Generic function that draw one pixel
	void fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b, bool selected_buffer);

	const uint8_t seq_REG_CMD[16] = {
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS,
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS,
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS,
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TS_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS,
		GPIO_OUT_W1TC_ADDRESS};
	const uint8_t seq_REG_VAL[16] = {
		1 << RGBMATRIX_GPIO_A,
		1 << RGBMATRIX_GPIO_B,
		1 << RGBMATRIX_GPIO_A,
		1 << RGBMATRIX_GPIO_C,
		1 << RGBMATRIX_GPIO_A,
		1 << RGBMATRIX_GPIO_B,
		1 << RGBMATRIX_GPIO_A,
		1 << RGBMATRIX_GPIO_D,
		1 << RGBMATRIX_GPIO_A,
		1 << RGBMATRIX_GPIO_C,
		1 << RGBMATRIX_GPIO_B,
		1 << RGBMATRIX_GPIO_C,
		1 << RGBMATRIX_GPIO_A,
		1 << RGBMATRIX_GPIO_C,
		1 << RGBMATRIX_GPIO_B,
		1 << RGBMATRIX_GPIO_D};
	const uint16_t seq_ROW_ID[16] = {
		1*SEND_BUFFER_SIZE,
		3*SEND_BUFFER_SIZE,
		2*SEND_BUFFER_SIZE,
		6*SEND_BUFFER_SIZE,
		7*SEND_BUFFER_SIZE,
		5*SEND_BUFFER_SIZE,
		4*SEND_BUFFER_SIZE,
		12*SEND_BUFFER_SIZE,
		13*SEND_BUFFER_SIZE,
		9*SEND_BUFFER_SIZE,
		11*SEND_BUFFER_SIZE,
		15*SEND_BUFFER_SIZE,
		14*SEND_BUFFER_SIZE,
		10*SEND_BUFFER_SIZE,
		8*SEND_BUFFER_SIZE,
		0*SEND_BUFFER_SIZE};


};

extern ESP8266RGBMatrix display;

#endif /*ESP8266RGBMatrix_H*/
