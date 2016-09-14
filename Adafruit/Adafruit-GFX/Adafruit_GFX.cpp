/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "Adafruit_GFX.h"
#include "glcdfont.c"

void PrintToLcdWithBuf(char *buf, int size, char *format, ...);
#define BUTTONBUFSIZE 16
char buttonBuf[BUTTONBUFSIZE];

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

#ifndef min
 #define min(a,b) ((a < b) ? a : b)
#endif

#ifndef abs
  #define abs(a) ((a) < 0 ? -(a) : (a))
#endif

Adafruit_GFX::Adafruit_GFX(int16_t w, int16_t h):
  WIDTH(w), HEIGHT(h)
{
  _width    = WIDTH;
  _height   = HEIGHT;
  rotation  = 0;
  cursor_y  = cursor_x    = 0;
  textsize  = 1;
  textcolor = textbgcolor = 0xFFFF;
  wrap      = true;
  _cp437    = false;
}

// Draw a circle outline
void Adafruit_GFX::drawCircle(int16_t x0, int16_t y0, int16_t r,
    uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0  , y0+r, color);
  drawPixel(x0  , y0-r, color);
  drawPixel(x0+r, y0  , color);
  drawPixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void Adafruit_GFX::drawCircleHelper( int16_t x0, int16_t y0,
               int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void Adafruit_GFX::fillCircle(int16_t x0, int16_t y0, int16_t r,
                  uint16_t color) {
  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void Adafruit_GFX::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
    uint8_t cornername, int16_t delta, uint16_t color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

// Bresenham's algorithm - thx wikpedia
void Adafruit_GFX::drawLine(int16_t x0, int16_t y0,
                int16_t x1, int16_t y1,
                uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// Draw a rectangle
void Adafruit_GFX::drawRect(int16_t x, int16_t y,
                int16_t w, int16_t h,
                uint16_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y+h-1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x+w-1, y, h, color);
}

void Adafruit_GFX::drawFastVLine(int16_t x, int16_t y,
                 int16_t h, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x, y+h-1, color);
}

void Adafruit_GFX::drawFastHLine(int16_t x, int16_t y,
                 int16_t w, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x+w-1, y, color);
}

void Adafruit_GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                uint16_t color) {
  // Update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++) {
    drawFastVLine(i, y, h, color);
  }
}

void Adafruit_GFX::fillScreen(uint16_t color) {
  fillRect(0, 0, _width, _height, color);
}

// Draw a rounded rectangle
void Adafruit_GFX::drawRoundRect(int16_t x, int16_t y, int16_t w,
  int16_t h, int16_t r, uint16_t color) {
  // smarter version
  drawFastHLine(x+r  , y    , w-2*r, color); // Top
  drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x    , y+r  , h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// Fill a rounded rectangle
void Adafruit_GFX::fillRoundRect(int16_t x, int16_t y, int16_t w,
                 int16_t h, int16_t r, uint16_t color) {
  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// Draw a triangle
void Adafruit_GFX::drawTriangle(int16_t x0, int16_t y0,
                int16_t x1, int16_t y1,
                int16_t x2, int16_t y2, uint16_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void Adafruit_GFX::fillTriangle ( int16_t x0, int16_t y0,
                  int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2, uint16_t color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }
}

void Adafruit_GFX::drawBitmap(int16_t x, int16_t y,
                  const uint8_t *bitmap, int16_t w, int16_t h,
                  uint16_t color) {

  int16_t i, j, byteWidth = (w + 7) / 8;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        drawPixel(x+i, y+j, color);
      }
    }
  }
}

// Draw a 1-bit color bitmap at the specified x, y position from the
// provided bitmap buffer (must be PROGMEM memory) using color as the
// foreground color and bg as the background color.
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y,
            const uint8_t *bitmap, int16_t w, int16_t h,
            uint16_t color, uint16_t bg) {

  int16_t i, j, byteWidth = (w + 7) / 8;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        drawPixel(x+i, y+j, color);
      }
      else {
        drawPixel(x+i, y+j, bg);
      }
    }
  }
}

//Draw XBitMap Files (*.xbm), exported from GIMP,
//Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//C Array can be directly used with this function
void Adafruit_GFX::drawXBitmap(int16_t x, int16_t y,
                              const uint8_t *bitmap, int16_t w, int16_t h,
                              uint16_t color) {

  int16_t i, j, byteWidth = (w + 7) / 8;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (1 << (i % 8))) {
        drawPixel(x+i, y+j, color);
      }
    }
  }
}

#if ARDUINO >= 100
size_t Adafruit_GFX::write(uint8_t c) {
#else
void Adafruit_GFX::write(uint8_t c) {
#endif
  if (c == '\n') {
    cursor_y += textsize*8;
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else {
    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
    cursor_x += textsize*6;
    if (wrap && (cursor_x > (_width - textsize*6))) {
      cursor_y += textsize*8;
      cursor_x = 0;
    }
  }
#if ARDUINO >= 100
  return 1;
#endif
}

// Draw a character
void Adafruit_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
                uint16_t color, uint16_t bg, uint8_t size) {

  if((x >= _width)            || // Clip right
     (y >= _height)           || // Clip bottom
     ((x + 6 * size - 1) < 0) || // Clip left
     ((y + 8 * size - 1) < 0))   // Clip top
    return;

  if(!_cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior

  for (int8_t i=0; i<6; i++ ) {
    uint8_t line;
    if (i == 5)
      line = 0x0;
    else
      line = pgm_read_byte(font+(c*5)+i);
    for (int8_t j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, color);
        else {  // big size
          fillRect(x+(i*size), y+(j*size), size, size, color);
        }
      } else if (bg != color) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, bg);
        else {  // big size
          fillRect(x+i*size, y+j*size, size, size, bg);
        }
      }
      line >>= 1;
    }
  }
}

void Adafruit_GFX::setCursor(int16_t x, int16_t y) {
  cursor_x = x;
  cursor_y = y;
}

int16_t Adafruit_GFX::getCursorX(void) const {
  return cursor_x;
}

int16_t Adafruit_GFX::getCursorY(void) const {
  return cursor_y;
}

void Adafruit_GFX::setTextSize(uint8_t s) {
  textsize = (s > 0) ? s : 1;
}

void Adafruit_GFX::setTextColor(uint16_t c) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}

void Adafruit_GFX::setTextColor(uint16_t c, uint16_t b) {
  textcolor   = c;
  textbgcolor = b;
}

void Adafruit_GFX::setTextWrap(boolean w) {
  wrap = w;
}

uint8_t Adafruit_GFX::getRotation(void) const {
  return rotation;
}

void Adafruit_GFX::setRotation(uint8_t x) {
  rotation = (x & 3);
  switch(rotation) {
   case 0:
   case 2:
    _width  = WIDTH;
    _height = HEIGHT;
    break;
   case 1:
   case 3:
    _width  = HEIGHT;
    _height = WIDTH;
    break;
  }
}

// Enable (or disable) Code Page 437-compatible charset.
// There was an error in glcdfont.c for the longest time -- one character
// (#176, the 'light shade' block) was missing -- this threw off the index
// of every character that followed it.  But a TON of code has been written
// with the erroneous character indices.  By default, the library uses the
// original 'wrong' behavior and old sketches will still work.  Pass 'true'
// to this function to use correct CP437 character values in your code.
void Adafruit_GFX::cp437(boolean x) {
  _cp437 = x;
}

// Return the size of the display (per current rotation)
int16_t Adafruit_GFX::width(void) const {
  return _width;
}

int16_t Adafruit_GFX::height(void) const {
  return _height;
}

void Adafruit_GFX::invertDisplay(boolean i) {
  // Do nothing, must be subclassed if supported
}

/***************************************************************************/
// code for the GFX button UI element

Adafruit_GFX_Button::Adafruit_GFX_Button(void) {
   _gfx = 0;
}

void Adafruit_GFX_Button::initButton(Adafruit_GFX *gfx,
                      int16_t x, int16_t y,
                      uint8_t w, uint8_t h,
                      uint16_t outline, uint16_t fill,
                      uint16_t textcolor,
                      char *label, uint8_t textsize)
{
  _x = x;
  _y = y;
  _w = w;
  _h = h;
  _outlinecolor = outline;
  _fillcolor = fill;
  _textcolor = textcolor;
  _textsize = textsize;
  _gfx = gfx;
  strncpy(_label, label, 9);
  _label[9] = 0;
}



 void Adafruit_GFX_Button::drawButton(boolean inverted) {
   uint16_t fill, outline, text;

   if (! inverted) {
     fill = _fillcolor;
     outline = _outlinecolor;
     text = _textcolor;
   } else {
     fill =  _textcolor;
     outline = _outlinecolor;
     text = _fillcolor;
   }

   _gfx->fillRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, fill);
   _gfx->drawRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, outline);


   _gfx->setCursor(_x - strlen(_label)*3*_textsize, _y-4*_textsize);
   _gfx->setTextColor(text);
   _gfx->setTextSize(_textsize);
   //_gfx->print(_label);
   PrintToLcdWithBuf(buttonBuf, BUTTONBUFSIZE, _label);
   //while(1); // Need to implement print
 }

 void Adafruit_GFX_Button::updateText
    (
    char*   label,
    boolean inverted
    )
{
    if( 0 != strncmp(_label, label, 9) )
    {
        uint16_t fill, outline, text;

        if (! inverted)
        {
            fill = _fillcolor;
            outline = _outlinecolor;
            text = _textcolor;
        }
        else
        {
            fill =  _textcolor;
            outline = _outlinecolor;
            text = _fillcolor;
        }

        _gfx->fillRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, fill);
        _gfx->drawRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, outline);

        strncpy(_label, label, 9);

        _gfx->setCursor(_x - strlen(_label)*3*_textsize, _y-4*_textsize);
        _gfx->setTextColor(text);
        _gfx->setTextSize(_textsize);


        PrintToLcdWithBuf(buttonBuf, BUTTONBUFSIZE, _label);
    }

   //while(1); // Need to implement print
 }

boolean Adafruit_GFX_Button::contains(int16_t x, int16_t y) {
   if ((x < (_x - _w/2)) || (x > (_x + _w/2))) return false;
   if ((y < (_y - _h/2)) || (y > (_y + _h/2))) return false;
   return true;
 }


 void Adafruit_GFX_Button::press(boolean p) {
   laststate = currstate;
   currstate = p;
 }

 boolean Adafruit_GFX_Button::isPressed() { return currstate; }
 boolean Adafruit_GFX_Button::justPressed() { return (currstate && !laststate); }
 boolean Adafruit_GFX_Button::justReleased() { return (!currstate && laststate); }
/***************************************************************************/
// code for the GFX List UI element

Adafruit_GFX_List::Adafruit_GFX_List(void)
{
   m_Gfx = 0;
}

/**
    Initialize the GFX list element
*/
void Adafruit_GFX_List::InitList
    (
    Adafruit_GFX *  a_Gfx,
    int16_t         a_X,
    int16_t         a_Y,
    uint8_t         a_W,
    uint8_t         a_H
    )
{
    uint8_t i;

    m_Gfx           = a_Gfx;
    m_X             = a_X,
    m_Y             = a_Y;
    m_W             = a_W;
    m_H             = a_H;
    m_CurSelIdx     = -1;
    m_MaxIdx        = -1;

    for( i =0; i < c_LIST_ITEM_CNT_MAX; i++ )
    {
        m_Text[i][0] = '\0';
    }
}

/**
    Draw the GFX list
*/
void Adafruit_GFX_List::DrawList
    ( void )
{
    int8_t i;
    int16_t y;

    y = m_Y;

    // Iterate through the list
    for( i = 0; i < c_LIST_ITEM_CNT_MAX; i++ )
    {
        // Draw the valid items
        if( i < m_MaxIdx )
        {
            m_Gfx->fillRect( m_X, y, m_W, m_H, c_LIST_ITEM_CLR_BLACK);
            m_Gfx->drawRect( m_X, y, m_W, m_H, c_LIST_ITEM_CLR_WHITE);
            m_Gfx->setCursor( m_X + c_LIST_TEXT_X_START_OFFSET, y + c_LIST_TEXT_Y_START_OFFSET );
            m_Gfx->setTextColor (c_LIST_ITEM_CLR_WHITE );
            m_Gfx->setTextSize( c_LIST_TEXT_SIZE );
            PrintToLcdWithBuf( buttonBuf, BUTTONBUFSIZE, m_Text[i] );
        }

        y += c_LIST_ITEM_Y_SPACING;
    }
}

/**
    Add item to the GFX list
*/
boolean Adafruit_GFX_List::AddItem( const char* a_PtrString )
{
    boolean success = false;

    if( ( m_MaxIdx < c_LIST_ITEM_CNT_MAX                    ) &&
        ( a_PtrString != NULL                               ) &&
        ( strlen( a_PtrString ) < c_LIST_ITEM_STR_LEN_MAX   )
      )
    {
        if( -1 == m_MaxIdx )
        {
            m_MaxIdx = 0;
        }
        strncpy( &(m_Text[m_MaxIdx][0]), a_PtrString, c_LIST_ITEM_STR_LEN_MAX );
        m_MaxIdx += 1;
        success = true;
    }

    return success;
}

/**
    Determine which list item was selected based on the
    input coordinates
*/

int8_t Adafruit_GFX_List::GetSelectedIndex( int16_t a_X, int16_t a_Y )
{
    int8_t index = -1;
    int8_t i;
    int16_t y = m_Y;

    // Iterate through the list and determine each
    // items bound and see if the coordinates fall
    // within the itesm bounds
    for( i = 0; i < m_MaxIdx; i++ )
    {
        if( ( ( a_X >=  m_X ) && ( a_X <= ( m_X + m_W ) ) ) &&
            ( ( a_Y >=  y   ) && ( a_Y <= ( y + m_H   ) ) )
          )
        {
            index = i;
            SetSelectedIndex( index );
            break;
        }

        y += c_LIST_ITEM_Y_SPACING;
    }

   return index;
}

/**
    Select a list item and draw a circle at the
    end indicating a selection
*/

void Adafruit_GFX_List::SetSelectedIndex( int8_t idx )
{
    int8_t i;

    for( i = 0; i < m_MaxIdx; i++ )
    {
        if( i != idx )
        {
            m_Gfx->fillCircle( m_W - 30, ( m_Y + ( i * c_LIST_ITEM_Y_SPACING ) + m_H ) - 11, 5, c_LIST_ITEM_CLR_BLACK );
        }
        else
        {
            m_Gfx->fillCircle( m_W - 30, ( m_Y + ( i * c_LIST_ITEM_Y_SPACING ) + m_H ) - 11, 5, c_LIST_ITEM_CLR_WHITE );
        }
    }

    m_CurSelIdx = idx;

}

/**
    Get the currently selected index
*/

int8_t Adafruit_GFX_List::GetCurrentSelectedIndex() const
{
    return m_CurSelIdx;
}

/**
    Get the string associated with an index
*/

const char* Adafruit_GFX_List::GetText( int8_t a_Idx ) const
{
    if( a_Idx < m_MaxIdx )
    {
        return m_Text[a_Idx];
    }
    else
    {
        return NULL;
    }

}

/**
    Get the next index from the currently selected index

    This function rolls over to the first index if the
    current selected index is at the last index in the list
*/

int8_t Adafruit_GFX_List::GetNextIndex() const
{
    if( m_CurSelIdx != -1 )
    {
        return ( ( m_CurSelIdx + 1 ) % m_MaxIdx );
    }
    else
    {
        return -1;
    }
}

/**
    Get the previous index from the currently selected index

    This function returns the last index in the list
    if the current selected index is the first item in the
    list
*/

int8_t Adafruit_GFX_List::GetPrevIndex() const
{
    if( m_CurSelIdx != -1 )
    {
        if( 0 == m_CurSelIdx )
        {
            return ( m_MaxIdx - 1 );
        }
        else
        {
            return ( m_CurSelIdx - 1 );
        }
    }
    else
    {
        return -1;
    }
}
