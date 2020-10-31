/*********************************************************************
This is a library for Chinese LED matrix displays

Written by Dominic Buchstaller.
BSD license, check license.txt for more information
*********************************************************************/


#ifndef _PxMATRIX_H
#define _PxMATRIX_H

// Pins affectation
#define GPIO_A 0
#define GPIO_B 4
#define GPIO_C 5
#define GPIO_D 2
#define GPIO_OE 15
#define GPIO_LAT 12

// Pixels width size
#ifndef PxMATRIX_WIDTH
#define PxMATRIX_WIDTH 32
#endif

// Pixels height size
#ifndef PxMATRIX_HEIGHT
#define PxMATRIX_HEIGHT 32
#endif

#define PxMATRIX_ROW_PATTERN 	16

// Color depth per primary color - the more the slower the update
#ifndef PxMATRIX_COLOR_DEPTH
#define PxMATRIX_COLOR_DEPTH 4
#endif

#if PxMATRIX_COLOR_DEPTH > 8 || PxMATRIX_COLOR_DEPTH < 1
#error "PxMATRIX_COLOR_DEPTH must be 1 to 8 bits maximum"
#endif

// Defines how long we display things by default
#ifndef PxMATRIX_SHOWTIME
#define PxMATRIX_SHOWTIME 10
#endif

// Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
#ifndef PxMATRIX_SPI_FREQUENCY
#define PxMATRIX_SPI_FREQUENCY 20000000
#endif

// Legacy suppport
#ifdef double_buffer
#define PxMATRIX_DOUBLE_BUFFER true
#endif

#ifndef _BV
#define _BV(x) (1 << (x))
#endif

#define SPI_TRANSFER(x,y) SPI.writeBytes(x,y)
#define SPI_BYTE(x) SPI.write(x)
#define SPI_2BYTE(x) SPI.write16(x)

#include "Adafruit_GFX.h"
#include "Arduino.h"
#include <SPI.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <stdlib.h>

// Sometimes some extra width needs to be passed to Adafruit GFX constructor
// to render text close to the end of the display correctly
#ifndef ADAFRUIT_GFX_EXTRA
#define ADAFRUIT_GFX_EXTRA 0
#endif


#define GPIO_REG_SET(val) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS,val)
#define GPIO_REG_CLEAR(val) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS,val)


// Specifies what blocking pattern the panel is using 
// |AB|,|DB|
// |CD|,|CA|
// |AB|,|DB|
// |CD|,|CA|
enum block_patterns {ABCD, DBCA};

// This is how the scanning is implemented. LINE just scans it left to right,
// ZIGZAG jumps 4 rows after every byte, ZAGGII alse revereses every second byte
enum scan_patterns {LINE, ZIGZAG,ZZAGG, ZAGGIZ, WZAGZIG, VZAG, ZAGZIG, WZAGZIG2, ZZIAGG};

// Specify the color order
enum color_orders {RRGGBB, RRBBGG, GGRRBB, GGBBRR, BBRRGG, BBGGRR};


#define color_step (256 / PxMATRIX_COLOR_DEPTH)
#define color_half_step (int(color_step / 2))
#define color_third_step (int(color_step / 3))
#define color_two_third_step (int(color_third_step*2))

// Total number of bytes that is pushed to the display at a time
// 3 * _pattern_color_bytes
//#define	BUFFER_SIZE				PxMATRIX_WIDTH*3/8
//#define PATTERN_COLOR_BYTES 	(PxMATRIX_HEIGHT/PxMATRIX_ROW_PATTERN)*(PxMATRIX_WIDTH/8)
//#define SEND_BUFFER_SIZE		PATTERN_COLOR_BYTES*3

class PxMATRIX : public Adafruit_GFX {
 public:
  inline PxMATRIX(uint16_t width, uint16_t height);
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B);
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C);
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D);
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D,uint8_t E);

  inline void begin(uint8_t row_pattern, uint8_t CLK, uint8_t MOSI, uint8_t MISO, uint8_t SS);
  inline void begin(uint8_t row_pattern);
  inline void begin();

  inline void clearDisplay(void);
  inline void clearDisplay(bool selected_buffer);

	inline void display_enable();
	inline void display_disable();
	inline uint32_t testCycle();
	inline void serialCycleCount();
	inline void serialInfo();

  // Draw pixels
  inline void drawPixelRGB565(int16_t x, int16_t y, uint16_t color);

  inline void drawPixel(int16_t x, int16_t y, uint16_t color);

  inline void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b);

  // Does nothing for now (always returns 0)
  uint8_t getPixel(int8_t x, int8_t y);

  // Converts RGB888 to RGB565
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  // FLush the buffer of the display
  inline void flushDisplay();

  // Rotate display
  inline void setRotate(bool rotate);

  // Flip display
  inline void setFlip(bool flip);
  
  // When using double buffering, this displays the draw buffer
  inline void showBuffer();

#ifdef PxMATRIX_DOUBLE_BUFFER
  // This copies the display buffer to the drawing buffer (or reverse)
  inline void copyBuffer(bool reverse);
#endif

  // Control the minimum color values that result in an active pixel
  inline void setColorOffset(uint8_t r, uint8_t g,uint8_t b);

    // Set the color order
  inline void setColorOrder(color_orders color_order);

  // Set the multiplex pattern {LINE, ZIGZAG, ZAGGIZ, WZAGZIG, VZAG, WZAGZIG2} (default is LINE)
  inline void setScanPattern(scan_patterns scan_pattern);

 // Set the block pattern {ABCD, DBCA} (default is ABCD)
  inline void setBlockPattern(block_patterns block_pattern);

  // Set the number of panels that make up the display area width (default is 1)
  inline void setPanelsWidth(uint8_t panels);

  // Set the brightness of the panels (default is 255)
  inline void setBrightness(uint8_t brightness);

 private:

 // the display buffer for the LED matrix
  uint8_t *PxMATRIX_buffer;
#ifdef PxMATRIX_DOUBLE_BUFFER
  uint8_t *PxMATRIX_buffer2;
#endif

  uint16_t _width;
  uint16_t _height;
  uint8_t _panels_width;
  uint8_t _rows_per_buffer;
  uint8_t _row_sets_per_buffer;
  uint8_t _panel_width_bytes;

  // Color offset
  uint8_t _color_R_offset;
  uint8_t _color_G_offset;
  uint8_t _color_B_offset;

  // Panel Brightness
  uint8_t _brightness;

  // Color pattern that is pushed to the display
  uint8_t _display_color;

  // Holds some pre-computed values for faster pixel drawing
  uint32_t *_row_offset;

  // Holds the display row pattern type
  uint8_t _row_pattern;

  // Number of bytes in one color
  uint8_t _pattern_color_bytes;

  // Total number of bytes that is pushed to the display at a time
  // 3 * _pattern_color_bytes
  uint16_t _buffer_size;
  uint16_t _send_buffer_size;

  // This is for double buffering
  bool _active_buffer;

  // Display and color engine
  bool _rotate;
  bool _flip;

  //Holdes the color order
  color_orders _color_order;

  // Holds the scan pattern
  scan_patterns _scan_pattern;

  // Holds the block pattern
  block_patterns _block_pattern;

	uint8_t _display_row = 0;
	uint8_t temp = 0;
	uint16_t _latch_seq[PxMATRIX_COLOR_DEPTH];
	static PxMATRIX *_instance;
	static void displayCallback();
	inline void displayTimer();
	inline void displayTimerChrono();
	inline void initLatchSeq();
	
	uint32_t cycleTotalTemp = 0;
	uint32_t cycleTotal = 0;
	uint32_t cycleTotalMin = 0xFFFFFFFF;
	uint32_t cycleTotalMax = 0;
/*
	struct struct_display_seq {
				uint8_t reg_cmd;
				uint8_t reg_val;
				uint8_t row;
			} ;
	struct_display_seq display_seq[16] ={ 
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_A, 1},
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_B, 3},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_A, 2},
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_C, 6},
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_A, 7},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_B, 5},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_A, 4},
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_D, 12},
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_A, 13},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_C, 9},
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_B, 11},
		{GPIO_OUT_W1TS_ADDRESS, 1<<GPIO_C, 15},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_A, 14},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_C, 10},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_B, 8},
		{GPIO_OUT_W1TC_ADDRESS, 1<<GPIO_D, 0}
	};
	struct_display_seq * display_seq_current = &display_seq[0];
*/		
	uint8_t seq_REG_CMD[16] = {
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
	uint8_t seq_REG_VAL[16] = {
		1<<GPIO_A,
		1<<GPIO_B,
		1<<GPIO_A,
		1<<GPIO_C,
		1<<GPIO_A,
		1<<GPIO_B,
		1<<GPIO_A,
		1<<GPIO_D,
		1<<GPIO_A,
		1<<GPIO_C,
		1<<GPIO_B,
		1<<GPIO_C,
		1<<GPIO_A,
		1<<GPIO_C,
		1<<GPIO_B,
		1<<GPIO_D};
	uint8_t seq_ROW_ID[16] = {
//		0,
		1,
		3,
		2,
		6,
		7,
		5,
		4,
		12,
		13,
		9,
		11,
		15,
		14,
		10,
		8,
		0};


  // Generic function that draw one pixel
inline void fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b, bool selected_buffer);

  // Init code common to both constructors
inline void init(uint16_t width, uint16_t height);

inline void spi_init();

// Write configuration register in some driver chips
inline void writeRegister(uint16_t reg_value, uint8_t reg_position);
inline void fm612xWriteRegister(uint16_t reg_value, uint8_t reg_position);
};

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
inline uint16_t PxMATRIX::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Init code common to both constructors
inline void PxMATRIX::init(uint16_t width, uint16_t height){
	_instance = this;

	_display_row = 0;
	_display_color=0;

  _width = width;
  _height = height;

  _buffer_size = (_width*_height * 3 / 8);

  _brightness=255;
  _panels_width = 1;

  _rows_per_buffer = _height/2;
  _panel_width_bytes = (_width/_panels_width)/8;

  _active_buffer=false;

  _color_R_offset=0;
  _color_G_offset=0;
  _color_B_offset=0;

  _rotate=0;
  _flip=0;

  _row_pattern=0;
  _scan_pattern=LINE;
  _block_pattern=ABCD;
  	
	initLatchSeq();

    PxMATRIX_buffer= new uint8_t[PxMATRIX_COLOR_DEPTH*_buffer_size];
  #ifdef PxMATRIX_DOUBLE_BUFFER
    PxMATRIX_buffer2=new uint8_t[PxMATRIX_COLOR_DEPTH*_buffer_size];
  #endif

}

inline void PxMATRIX::setColorOrder(color_orders color_order)
{
  _color_order=color_order;
}

inline void PxMATRIX::setScanPattern(scan_patterns scan_pattern)
{
  _scan_pattern=scan_pattern;
}

inline void PxMATRIX::setBlockPattern(block_patterns block_pattern)
{
  _block_pattern=block_pattern;
}

inline void PxMATRIX::setPanelsWidth(uint8_t panels) {
  _panels_width=panels;
  _panel_width_bytes = (_width/_panels_width)/8;
}

inline void PxMATRIX::setRotate(bool rotate) {
  _rotate=rotate;
}

inline void PxMATRIX::setFlip(bool flip) {
  _flip=flip;
}

inline void PxMATRIX::setBrightness(uint8_t brightness) {
  _brightness=brightness;
	initLatchSeq();
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  init(width, height);
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  init(width, height);
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  init(width, height);
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  init(width, height);
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D, uint8_t E) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  init(width, height);
}

inline void PxMATRIX::drawPixel(int16_t x, int16_t y, uint16_t color) {
  drawPixelRGB565(x, y, color);
}

inline void PxMATRIX::showBuffer() {
  _active_buffer=!_active_buffer;
}

#ifdef PxMATRIX_DOUBLE_BUFFER
inline void PxMATRIX::copyBuffer(bool reverse = false) {
  // This copies the display buffer to the drawing buffer (or reverse)
  // You may need this in case you rely on the framebuffer to always contain the last frame
  // _active_buffer = true means that PxMATRIX_buffer2 is displayed
  if(_active_buffer ^ reverse) {
    memcpy(PxMATRIX_buffer, PxMATRIX_buffer2, PxMATRIX_COLOR_DEPTH*_buffer_size);
  } else {
    memcpy(PxMATRIX_buffer2, PxMATRIX_buffer, PxMATRIX_COLOR_DEPTH*_buffer_size);
  }
}
#endif

inline void PxMATRIX::setColorOffset(uint8_t r, uint8_t g,uint8_t b)
{
    _color_R_offset=r;
    _color_G_offset=g;
    _color_B_offset=b;
}

inline void PxMATRIX::fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b,bool selected_buffer)
{
 

  if (r>_color_R_offset)
    r-=_color_R_offset;
  else
    r=0;
  if (g>_color_G_offset)
    g-=_color_G_offset;
  else  
    g=0;
  if (b>_color_B_offset)
    b-=_color_B_offset;
  else  
    b=0;
   
  if (_block_pattern==DBCA)
  {
    
    
      // Every matrix is segmented in 8 blocks - 2 in X, 4 in Y direction
      // |AB|
      // |CD|
      // |AB|
      // |CD|
      // Have to rewrite this block suff and move to the scan pattern section - this will only work for chaining up to 2 panels
      
      if (_panels_width>1) // Only works for two panels
      {
        if ((x>=_width/4) && (x<_width/2))
          x+=_width/4;
        else if ((x>=_width/2) && (x<_width*3/4))
          x-=_width/4;
      }

      uint16_t y_block = y*4/_height;
      uint16_t x_block = x*2*_panels_width/_width;

      // Swapping A & D
      if (!(y_block%2)) // Even y block
      {
        if (!(x_block%2)) // Left side of panel
        {
          x+=_width/2/_panels_width;
          y+=_height/4;
         }
      }
      else // Odd y block
      {
        if (x_block%2) // Right side of panel
        {
          x-=_width/2/_panels_width;
          y-=_height/4;
         
        } 
      }
     
  }

    if (_rotate){
    uint16_t temp_x=x;
    x=y;
    y=_height-1-temp_x;
  }

  // Panels are naturally flipped
  if (!_flip)
    x =_width - 1 -x;
  
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
    return; 
  
  if (_color_order!= RRGGBB)
  {
    uint8_t r_temp=r;
    uint8_t g_temp=g;
    uint8_t b_temp=b;


    switch (_color_order)
    {
      case (RRGGBB): break;
      case (RRBBGG): g=b_temp; b=g_temp; break;
      case (GGRRBB): r=g_temp; g=r_temp; break;
      case (GGBBRR): r=g_temp; g=b_temp; b=r_temp; break;
      case (BBRRGG): r=b_temp; g=r_temp; b=g_temp; break;
      case (BBGGRR): r=b_temp; g=g_temp; b=r_temp; break;
    }
  }
  
  uint32_t base_offset;
  uint32_t total_offset_r=0;
  uint32_t total_offset_g=0;
  uint32_t total_offset_b=0;

  if (_scan_pattern==WZAGZIG || _scan_pattern==VZAG || _scan_pattern==WZAGZIG2)
  {
    // get block coordinates and constraints
    uint8_t rows_per_buffer = _height/2;
    uint8_t rows_per_block = rows_per_buffer/2;
    // this is a defining characteristic of WZAGZIG and VZAG:
    // two byte alternating chunks bottom up for WZAGZIG
    // two byte up down down up for VZAG
    uint8_t cols_per_block = 16;
    uint8_t panel_width = _width/_panels_width;
    uint8_t blocks_x_per_panel = panel_width/cols_per_block;
    uint8_t panel_index = x/panel_width;
    // strip down to single panel coordinates, restored later using panel_index
    x = x%panel_width;
    uint8_t base_y_offset = y/rows_per_buffer;
    uint8_t buffer_y = y%rows_per_buffer;
    uint8_t block_x = x/cols_per_block;
    uint8_t block_x_mod = x%cols_per_block;
    uint8_t block_y = buffer_y/rows_per_block; // can only be 0/1 for height/pattern=4
    uint8_t block_y_mod = buffer_y%rows_per_block;

    // translate block address to new block address
    // invert block_y so remaining translation will be more sane
    uint8_t block_y_inv = 1 - block_y;
    uint8_t block_x_inv = blocks_x_per_panel - block_x - 1;
    uint8_t block_linear_index;
    if (_scan_pattern==WZAGZIG2) {
      block_linear_index = block_x_inv * 2 + block_y;
    }
    else if (_scan_pattern==WZAGZIG)
    {
      // apply x/y block transform for WZAGZIG, only works for height/pattern=4
      block_linear_index = block_x_inv * 2 + block_y_inv;
    }
    else if (_scan_pattern==VZAG)
    {
      // apply x/y block transform for VZAG, only works for height/pattern=4 and 32x32 panels until a larger example is found
      block_linear_index = block_x_inv * 3 * block_y + block_y_inv  * (block_x_inv + 1);
    }
    // render block linear index back into normal coordinates
    uint8_t new_block_x = block_linear_index % blocks_x_per_panel;
    uint8_t new_block_y = 1 - block_linear_index/blocks_x_per_panel;
    x = new_block_x * cols_per_block + block_x_mod + panel_index * panel_width;
    y = new_block_y * rows_per_block + block_y_mod + base_y_offset * rows_per_buffer;
  }

  // This code sections computes the byte in the buffer that will be manipulated.
  if (_scan_pattern!=LINE && _scan_pattern!=WZAGZIG && _scan_pattern!=VZAG && _scan_pattern!=WZAGZIG2)
  {
    // Precomputed row offset values
    base_offset=_row_offset[y]-(x/8)*2;
    uint8_t row_sector=0;
    uint16_t row_sector__offset=_width/4;
    for (uint8_t yy = 0; yy<_height; yy+=2*_row_pattern)
    {
      if ((yy<=y) && (y<yy+_row_pattern))
        total_offset_r=base_offset-row_sector__offset*row_sector;
      if ((yy+_row_pattern<=y) && (y<yy+2*_row_pattern))
        total_offset_r=base_offset-row_sector__offset*row_sector;

      row_sector++;
    }
  }
  else
  {
    // can only be non-zero when _height/(2 inputs per panel)/_row_pattern > 1
    // i.e.: 32x32 panel with 1/8 scan (A/B/C lines) -> 32/2/8 = 2
    uint8_t vert_index_in_buffer = (y%_rows_per_buffer)/_row_pattern; // which set of rows per buffer

    // can only ever be 0/1 since there are only ever 2 separate input sets present for this variety of panels (R1G1B1/R2G2B2)
    uint8_t which_buffer = y/_rows_per_buffer;
    uint8_t x_byte = x/8;
    // assumes panels are only ever chained for more width
    uint8_t which_panel = x_byte/_panel_width_bytes;
    uint8_t in_row_byte_offset = x_byte%_panel_width_bytes;
    // this could be pretty easily extended to vertical stacking as well
    total_offset_r = _row_offset[y] - in_row_byte_offset - _panel_width_bytes*(_row_sets_per_buffer*(_panels_width*which_buffer + which_panel) + vert_index_in_buffer);
  }


  uint8_t bit_select = x%8;

  
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
  if ((y%(_row_pattern*2))<_row_pattern)
  {
    // Variant of ZAGZIG pattern with bit oder reversed on lower part (starts on upper part)
    if (_scan_pattern==ZAGGIZ)
    {
        total_offset_r--;
        bit_select = 7-bit_select;
    }

    // Row changing pattern (starts on upper part)
    if (_scan_pattern==ZAGZIG)
      total_offset_r--;

    // Byte split pattern - like ZAGZIG but after every 4 bit (starts on upper part)     
    if (_scan_pattern == ZZIAGG )
    {
      if (bit_select>3)
      
         bit_select +=4;
      else
        total_offset_r--;   
    }

    // Byte split pattern (lower part)
    if (_scan_pattern==ZZAGG)
        if (bit_select>3) total_offset_r--;
  }
  else
  {

    if (_scan_pattern==ZIGZAG)
      total_offset_r--;

       // Byte split pattern - like ZAGZIG but after every 4 bit (starts on upper part)  
    if (_scan_pattern == ZZIAGG )
    {

      if (bit_select>3)
      {
        total_offset_r--;
        bit_select -=4;
      }
    }

    // Byte split pattern (upper part)
    if (_scan_pattern==ZZAGG)
    {
      if (bit_select<=3)
        bit_select += 4;
      else
      {
        bit_select -=4;
        total_offset_r--;
      }
    }
  }

  total_offset_g=total_offset_r-_pattern_color_bytes;
  total_offset_b=total_offset_g-_pattern_color_bytes;

uint8_t *PxMATRIX_bufferp = PxMATRIX_buffer;

#ifdef PxMATRIX_DOUBLE_BUFFER
  PxMATRIX_bufferp = selected_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif

  r = r >> (8-PxMATRIX_COLOR_DEPTH);
  g = g >> (8-PxMATRIX_COLOR_DEPTH);
  b = b >> (8-PxMATRIX_COLOR_DEPTH);
  
  //Color interlacing
  for (int this_color_bit=0; this_color_bit<PxMATRIX_COLOR_DEPTH; this_color_bit++)
  {
    if ((r >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_r] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_r] &= ~_BV(bit_select);

    if ((g >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_g] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_g] &= ~_BV(bit_select);

    if ((b >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_b] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_b] &= ~_BV(bit_select);
  }
}

inline void PxMATRIX::drawPixelRGB565(int16_t x, int16_t y, uint16_t color) {
  uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
  uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
  uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;
#ifdef PxMATRIX_DOUBLE_BUFFER
  fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
  fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

inline void PxMATRIX::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
  fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

// the most basic function, get a single pixel
inline uint8_t PxMATRIX::getPixel(int8_t x, int8_t y) {
  return (0);//PxMATRIX_buffer[x+ (y/8)*LCDWIDTH] >> (y%8)) & 0x1;
}

inline void PxMATRIX::begin()
{
  begin(8);

}

void PxMATRIX::begin(uint8_t row_pattern, uint8_t CLK, uint8_t MOSI, uint8_t MISO, uint8_t SS)
{
  begin(row_pattern);
}

void PxMATRIX::spi_init(){
//	uint32_t mask = ~(SPIMMOSI << SPILMOSI);
//	uint16_t bits = _send_buffer_size*8 -1;

    SPI.begin();

    SPI.setFrequency(PxMATRIX_SPI_FREQUENCY);

    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
//	SPI1U1 = ((SPI1U1 & mask) | (bits << SPILMOSI));
}

void PxMATRIX::begin(uint8_t row_pattern) {



  _row_pattern=row_pattern;
  if (_row_pattern==4)
    _scan_pattern=ZIGZAG;
  _color_order=RRGGBB;

   
  _pattern_color_bytes=(_height/_row_pattern)*(_width/8);
  _row_sets_per_buffer = _rows_per_buffer/_row_pattern;
  _send_buffer_size=_pattern_color_bytes*3;



  spi_init();

  pinMode(GPIO_OE, OUTPUT);
  pinMode(GPIO_LAT, OUTPUT);
  pinMode(GPIO_A, OUTPUT);
  pinMode(GPIO_B, OUTPUT);
  pinMode(GPIO_C, OUTPUT);
  pinMode(GPIO_D, OUTPUT);
  
  digitalWrite(GPIO_OE, HIGH);
  digitalWrite(GPIO_LAT, LOW);
  digitalWrite(GPIO_A, LOW);
  digitalWrite(GPIO_B, LOW);
  digitalWrite(GPIO_C, LOW);
  digitalWrite(GPIO_D, LOW);

  // Precompute row offset values
  _row_offset=new uint32_t[_height];
  for (uint8_t yy=0; yy<_height;yy++)
      _row_offset[yy]=((yy)%_row_pattern)*_send_buffer_size+_send_buffer_size-1;

}

inline void PxMATRIX::initLatchSeq(){
	for(uint8_t i = 0; i <PxMATRIX_COLOR_DEPTH;i++)
		// Lacth = cycle de pause 
		// Latch = 5*durée(en microseconde) = durée(en nanoseconde)/200
		// 200 nanoseconde = durée d'un cycle de timer pour TIM_DIV16
		_latch_seq[i] = 5*((PxMATRIX_SHOWTIME*(1<<i)*_brightness)/255/2);
}

inline void PxMATRIX::display_enable(){
	noInterrupts();
	timer1_isr_init();
	timer1_attachInterrupt(PxMATRIX::displayCallback);
	timer1_enable(TIM_DIV16,TIM_EDGE,TIM_LOOP);
	timer1_write(5000); // 1ms
	interrupts();
}

inline void PxMATRIX::display_disable(){
	timer1_disable();
}

PxMATRIX *PxMATRIX::_instance = NULL;

inline void PxMATRIX::displayCallback() {
//	if (PxMATRIX::_instance) //Forcément affecté
//		PxMATRIX::_instance->displayTimer();
		PxMATRIX::_instance->displayTimerChrono();
}

/* asm-helpers */
static inline int32_t asm_ccount(void) {
    int32_t r; asm volatile ("rsr %0, ccount" : "=r"(r)); return r; }
	
inline uint32_t PxMATRIX::testCycle(){
	uint32_t deb = asm_ccount();
	displayTimer();
	return asm_ccount() - deb;
}

inline void PxMATRIX::displayTimerChrono(){
	noInterrupts();
	uint32_t deb = asm_ccount();
	displayTimer();
	cycleTotal = asm_ccount() - deb;
	interrupts();
	
	if (cycleTotal>cycleTotalMax)
		cycleTotalMax = cycleTotal;
	if (cycleTotal<cycleTotalMin)
		cycleTotalMin = cycleTotal;
	
/*
	// Calcul du temps total pour afficher une image compléte (16*Nbcouleur)
	cycleTotalTemp += ESP.getCycleCount() - deb;
	if (_display_row==0 && _display_color==0){
		cycleTotal = cycleTotalTemp;
		if (cycleTotal>cycleTotalMax)
			cycleTotalMax = cycleTotal;
		if (cycleTotal<cycleTotalMin)
			cycleTotalMin = cycleTotal;
		cycleTotalTemp = 0;
	}
	*/
}
inline void PxMATRIX::serialInfo(){
	Serial.printf("CPU : %d MHz (CPU2X=%x)\r\n", CPU2X & 1?160:80,CPU2X);
	Serial.println("Sequence : ");
	for(uint8_t i = 0; i <PxMATRIX_COLOR_DEPTH;i++)
		Serial.printf("\t%d (us) %d (cycles)\r\n", _latch_seq[i]/5, _latch_seq[i]*(CPU2X?160:80)/5);
}

inline void PxMATRIX::serialCycleCount(){
	Serial.printf("%d\t%d\t%d\r\n", cycleTotal, cycleTotalMin, cycleTotalMax);
	cycleTotalMin = 0xFFFF;
	cycleTotalMax = 0;
}

inline void PxMATRIX::displayTimer(){

//	asm volatile (" nop ");
	//voir eagle_soc.h
//  ESP.wdtFeed();
//	noInterrupts();
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS,1<<GPIO_OE);
	GPIO_REG_WRITE(seq_REG_CMD[_display_row],seq_REG_VAL[_display_row]);
//	GPIO_REG_WRITE(seq_REG_CMD[temp],seq_REG_VAL[temp]);
//	GPIO_REG_WRITE(display_seq_current->reg_cmd,display_seq_current->reg_val);
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS,1<<GPIO_LAT);
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS,1<<GPIO_LAT);
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS,1<<GPIO_OE);

	_display_row++;
//	temp = _display_row & 0xF;
//	display_seq_current = &display_seq[_display_row & 0xF];
 	if (_display_row == _row_pattern){
		_display_row = 0; //Calculer le pointeur pour éviter multiplication
		_display_color++;
		//_display_color = _display_row>>4;
		if (_display_color==PxMATRIX_COLOR_DEPTH){
			//_display_row = 0;
			_display_color=0;
		}
		timer1_write(_latch_seq[_display_color]);
	}

#ifdef PxMATRIX_DOUBLE_BUFFER
	uint8_t *PxMATRIX_bufferp = _active_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#else
	uint8_t *PxMATRIX_bufferp = PxMATRIX_buffer;
#endif
//	memcpy((void *)&SPI1W0,&PxMATRIX_bufferp[_display_color*_buffer_size+seq_ROW_ID[temp]*_send_buffer_size],_send_buffer_size);
//	memcpy((void *)&SPI1W0,&PxMATRIX_bufferp[_display_color*_buffer_size+display_seq_current->row*_send_buffer_size],_send_buffer_size);
//	SPI1CMD |= SPIBUSY;
//	SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+seq_ROW_ID[temp]*_send_buffer_size],_send_buffer_size);
	SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+seq_ROW_ID[_display_row]*_send_buffer_size],_send_buffer_size);
//	interrupts();
}

void PxMATRIX::flushDisplay(void) {
  for (int ii=0;ii<_send_buffer_size;ii++)
    SPI_BYTE(0x00);
}

void PxMATRIX::clearDisplay(void) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  clearDisplay(!_active_buffer);
#else
  clearDisplay(false);
#endif
}
//    void * memset ( void * ptr, int value, size_t num );

// clear everything
void PxMATRIX::clearDisplay(bool selected_buffer) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  if(selected_buffer)
    memset(PxMATRIX_buffer2, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
  else
    memset(PxMATRIX_buffer, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
#else
    memset(PxMATRIX_buffer, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
#endif
}

#endif