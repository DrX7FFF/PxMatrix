#ifndef ESP8266RGBMatrix_H
#define ESP8266RGBMatrix_H

#define DEBUG_RGBMatrix  //Uncomment this to enable debug messages over serial port
// #define GPIO_OE 15
// #define GPIO_LAT 12
// #define GPIO_A 0
// #define GPIO_B 4
// #define GPIO_C 5
// #define GPIO_D 2
// #define GPIO_E ?

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

#define RGBMATRIX_DOUBLE_BUFFER true

// Default values
#define RGBMATRIX_DEFAULT_COLOR_DEPTH 6


// Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
#ifndef RGBMATRIX_SPI_FREQUENCY
#define RGBMATRIX_SPI_FREQUENCY 20000000
#endif

// Helper
#define PATTERN_COLOR_BYTES ((RGBMATRIX_HEIGHT / RGBMATRIX_ROW_PATTERN) * (RGBMATRIX_WIDTH / 8))
#define SEND_BUFFER_SIZE (PATTERN_COLOR_BYTES * 3)

#ifndef _BV
#define _BV(x) (1 << (x))
#endif
#define D2B(v)		((((v&0b1)==0b1)?1:0)+(((v&0b10)==0b10)?10:0)+(((v&0b100)==0b100)?100:0)+(((v&0b1000)==0b1000)?1000:0)+(((v&0b10000)==0b10000)?10000:0)+(((v&0b100000)==0b100000)?100000:0)+(((v&0b1000000)==0b1000000)?1000000:0)+(((v&0b10000000)==0b10000000)?10000000:0))


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
	ESP8266RGBMatrix();
	void setGPIO(uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C);
	void setGPIO(uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C, uint8_t gpio_D);
	void setGPIO(uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C, uint8_t gpio_D, uint8_t gpio_E);
	void begin(uint8_t colorDepth);
	void setFramesPerSec(uint8_t frames);
	void disable();
	bool enable();
	static inline void refreshCallback();
	inline void refresh();
	void refreshTest();
	void clearDisplay();
	void clearDisplay(bool selected_buffer);
	//inline uint8_t* getEditBuffer() { return _edit_buffer; };
	void drawPixelRGB565(int16_t x, int16_t y, uint16_t color);	// Draw pixels
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
	uint8_t getPixel(int8_t x, int8_t y);                // Does nothing for now (always returns 0)
	uint16_t color565(uint8_t r, uint8_t g, uint8_t b);  // Converts RGB888 to RGB565
	void showBuffer();
	void copyBuffer(bool reverse);

	void setBrightness(uint8_t brightness);			// Set the brightness of the panels (default is 255)
	void setRotate(bool rotate);  					// Rotate display
	void setFlip(bool flip);      					// Flip display
	void setColorOrder(color_orders color_order);	// Set the color order
	void setScanPattern(scan_patterns scan_pattern);	// Set the multiplex pattern {LINE, ZIGZAG, ZAGGIZ, WZAGZIG, VZAG, WZAGZIG2} (default is LINE)
	void setBlockPattern(block_patterns block_pattern);	// Set the block pattern {ABCD, DBCA} (default is ABCD)
	void setColorOffset(uint8_t r, uint8_t g, uint8_t b);// Control the minimum color values that result in an active pixel
	void setPanelsWidth(uint8_t panels);			// Set the number of panels that make up the display area width (default is 1)

private:
	uint16_t _mask_OE;
	uint16_t _mask_LAT;
	uint16_t _mask_A;
	uint16_t _mask_B;
	uint16_t _mask_C;
	uint16_t _mask_D;
	uint16_t _mask_E;

	bool _doubleBuffer;
	uint8_t _framesPerSec;
	uint16_t _showTicks;
	uint8_t _colorDepth;
	uint8_t _muxBits;
	uint8_t _rowPattern;
	uint32_t _bufferSize;
	uint32_t _patternColorBytes;
	uint32_t _sendBufferSize;
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
	uint8_t _display_layer;

	// Holds some pre-computed values for faster pixel drawing
	uint32_t* _row_offset;

	//Gestion des buffers
	uint8_t* _buffer;
	uint8_t* _display_buffer;
	uint8_t* _display_buffer_pos;
#ifdef RGBMATRIX_DOUBLE_BUFFER
	uint8_t* _buffer2;
	//uint8_t* _edit_buffer;
	bool _active_buffer;
#else
	//uint8_t* _edit_buffer = _buffer;
#endif

	void init_SPIBufferSize();
	void initShowTicks();
	void initPatternSeq();
	void initPreIndex();
	void initGPIO(uint8_t muxBits, uint8_t gpio_OE, uint8_t gpio_LAT, uint8_t gpio_A, uint8_t gpio_B, uint8_t gpio_C, uint8_t gpio_D, uint8_t gpio_E);
	// Generic function that draw one pixel
	void fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b, bool selected_buffer);

	struct muxStruct {
		uint8_t cmd;
		uint16_t val;
		uint16_t offset;
	} ;
	muxStruct* _muxSeq;
};

extern ESP8266RGBMatrix display;

#endif /*ESP8266RGBMatrix_H*/
