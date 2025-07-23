#pragma once

#include "Display.h"
#include <Wire.h>

class Adafruit_SH1107;

class SH1107Display : public Display {
public:
    SH1107Display();
    ~SH1107Display() override;

    void begin() override;
    void setRotation(uint8_t rotation) override;
    int width() override;
    int height() override;
    void fillScreen(uint16_t color) override;
    void drawRGBBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h) override;
    void setCursor(int16_t x, int16_t y) override;
    void setTextColor(uint16_t color, uint16_t bg) override;
    void setFont(DisplayFont font) override;
    void setTextSize(uint8_t size) override;
    void println(const String &s) override;
    void print(const String &s) override;
    void printf(const char *format, ...) override;
    int16_t getCursorY() override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
    void getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) override;
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) override;
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override;
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) override;
    void flush() override;

private:
    Adafruit_SH1107* _display;
};
