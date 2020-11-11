#ifndef RGBMatrixDraw_H
#define RGBMatrixDraw_H

#include "Adafruit_GFX.h"
#include "ESP8266RGBMatrix.h"

// Sometimes some extra width needs to be passed to Adafruit GFX constructor
// to render text close to the end of the display correctly
#ifndef ADAFRUIT_GFX_EXTRA
#define ADAFRUIT_GFX_EXTRA 0
#endif

class RGBMatrixDraw : public Adafruit_GFX {
public:
	RGBMatrixDraw(int16_t width, int16_t height);
	uint16_t color565(uint8_t r, uint8_t g, uint8_t b);  // Converts RGB888 to RGB565
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	void drawPixelRGB565(int16_t x, int16_t y, uint16_t color);	// Draw pixels
	void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
};
#endif /*RGBMatrixDraw_H*/