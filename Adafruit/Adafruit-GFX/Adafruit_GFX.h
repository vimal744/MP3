#ifndef _ADAFRUIT_GFX_H
#define _ADAFRUIT_GFX_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define boolean bool

#define swap(a, b) { int16_t t = a; a = b; b = t; }

class Adafruit_GFX {

 public:

  Adafruit_GFX(int16_t w, int16_t h); // Constructor

  // This MUST be defined by the subclass:
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;

  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void
    drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color),
    drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
    drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
    drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
    fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
    fillScreen(uint16_t color),
    invertDisplay(boolean i);

  // These exist only with Adafruit_GFX (no subclass overrides)
  void
    drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
    drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
      uint16_t color),
    fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
    fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
      int16_t delta, uint16_t color),
    drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color),
    fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color),
    drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color),
    fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color),
    drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color),
    drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color, uint16_t bg),
    drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
      int16_t w, int16_t h, uint16_t color),
    drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
      uint16_t bg, uint8_t size),
    setCursor(int16_t x, int16_t y),
    setTextColor(uint16_t c),
    setTextColor(uint16_t c, uint16_t bg),
    setTextSize(uint8_t s),
    setTextWrap(boolean w),
    setRotation(uint8_t r),
    cp437(boolean x=true);

#if ARDUINO >= 100
  virtual size_t write(uint8_t);
#else
  virtual void   write(uint8_t);
#endif

  int16_t height(void) const;
  int16_t width(void) const;

  uint8_t getRotation(void) const;

  // get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
  int16_t getCursorX(void) const;
  int16_t getCursorY(void) const;

 protected:
  const int16_t
    WIDTH, HEIGHT;   // This is the 'raw' display w/h - never changes
  int16_t
    _width, _height, // Display w/h as modified by current rotation
    cursor_x, cursor_y;
  uint16_t
    textcolor, textbgcolor;
  uint8_t
    textsize,
    rotation;
  boolean
    wrap,   // If set, 'wrap' text at right edge of display
    _cp437; // If set, use correct CP437 charset (default is off)
};

class Adafruit_GFX_Button {

 public:
  Adafruit_GFX_Button(void);
  void initButton(Adafruit_GFX *gfx, int16_t x, int16_t y,
              uint8_t w, uint8_t h,
              uint16_t outline, uint16_t fill, uint16_t textcolor,
              char *label, uint8_t textsize);
  void drawButton(boolean inverted = false);

  void updateText
    (
    char*   label,
    boolean inverted = false
    );


  boolean contains(int16_t x, int16_t y);

  void press(boolean p);
  boolean isPressed();
  boolean justPressed();
  boolean justReleased();

 private:
  Adafruit_GFX *_gfx;
  int16_t _x, _y;
  uint16_t _w, _h;
  uint8_t _textsize;
  uint16_t _outlinecolor, _fillcolor, _textcolor;
  char _label[10];

  boolean currstate, laststate;
};

class Adafruit_GFX_List
{

    public:
        Adafruit_GFX_List();

        void InitList
            (
            Adafruit_GFX *  a_Gfx,
            int16_t         a_X,
            int16_t         a_Y,
            uint8_t         a_W,
            uint8_t         a_H
            );

        void DrawList();

        boolean AddItem( const char* a_PtrString );

        int8_t GetSelectedIndex( int16_t a_X, int16_t a_Y );

        const char* GetText( int8_t a_Idx ) const;

        void SetSelectedIndex( int8_t idx );

        int8_t GetNextIndex() const;

        int8_t GetPrevIndex() const;

        int8_t GetCurrentSelectedIndex() const;

    private:

        static const int        c_LIST_ITEM_CNT_MAX         = 4;
        static const int        c_LIST_ITEM_STR_LEN_MAX     = 15;
        static const uint16_t   c_LIST_ITEM_CLR_BLACK       = 0x0000;
        static const uint16_t   c_LIST_ITEM_CLR_WHITE       = 0xFFFF;
        static const uint8_t    c_LIST_TEXT_X_START_OFFSET  = 3;       
        static const uint8_t    c_LIST_TEXT_Y_START_OFFSET  = 3;
        static const uint8_t    c_LIST_TEXT_SIZE            = 2;
        static const uint8_t    c_LIST_ITEM_Y_SPACING       = 30;

        Adafruit_GFX *m_Gfx;
        int16_t       m_X;
        int16_t       m_Y;
        uint16_t      m_W;
        uint16_t      m_H;
        int8_t        m_CurSelIdx;
        int8_t        m_MaxIdx;
        char          m_Text[c_LIST_ITEM_CNT_MAX][c_LIST_ITEM_STR_LEN_MAX];
};


#endif // _ADAFRUIT_GFX_H
