#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "GPSManager.h"

class Display {
public:
    virtual ~Display() = default;

    virtual void begin() = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    virtual int width() = 0;
    virtual int height() = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void draw24bitRGBBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void setTextColor(uint16_t color, uint16_t bg) = 0;
    virtual void setFont(const GFXfont *f) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void println(const String &s) = 0;
    virtual void print(const String &s) = 0;
    virtual void printf(const char *format, ...) = 0;
    virtual int16_t getCursorY() = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) = 0;
    virtual void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) = 0;
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
    virtual void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) = 0;
    virtual void flush() = 0;
};