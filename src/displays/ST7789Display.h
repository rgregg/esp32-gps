#pragma once

#include "Display.h"

class Arduino_DataBus;
class Arduino_GFX;

/// @brief Implements the common display functions for a color screen based on the ST7789 chip
class ST7789Display : public Display {
public:
    ST7789Display();
    ~ST7789Display() override;

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
    void setBacklight(uint8_t percent) override;


private:
    Arduino_DataBus* _bus;
    Arduino_GFX* _gfx;
    Arduino_GFX* _display;
};