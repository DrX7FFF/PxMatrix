#include "RGBMatrixDraw.h"

RGBMatrixDraw::RGBMatrixDraw(int16_t width, int16_t height) : Adafruit_GFX(width + ADAFRUIT_GFX_EXTRA, height) {

}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t RGBMatrixDraw::color565(uint8_t r, uint8_t g, uint8_t b) {
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void RGBMatrixDraw::drawPixel(int16_t x, int16_t y, uint16_t color) {
	drawPixelRGB565(x, y, color);
}

void RGBMatrixDraw::drawPixelRGB565(int16_t x, int16_t y, uint16_t color) {
	uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
	uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
	uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;
	RGBMatrix.setPixel(x, y, r, g, b);
}

void RGBMatrixDraw::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
	RGBMatrix.setPixel(x, y, r, g, b);
}