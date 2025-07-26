#ifdef USE_ST7789_DISPLAY

#include "ST7789Display.h"
#include "Constants.h"
#include <Arduino_GFX_Library.h>

#include "fonts/futura_medium_bt10pt8b.h"
#include "fonts/futura_medium_bt12pt8b.h"
#include "fonts/futura_medium_bt14pt8b.h"
#include "fonts/futura_medium_bt16pt8b.h"

#define BACKLIGHT_PWM_CHANNEL 0
#define BACKLIGHT_PWM_FREQ 5000
#define BACKLIGHT_PWM_RESOLUTION 8

ST7789Display::ST7789Display() {
    _bus = new Arduino_ESP32PAR8Q(
        SCREEN_DC_PIN, SCREEN_CS_PIN, SCREEN_WR_PIN, SCREEN_RD_PIN,
        SCREEN_D0_PIN, SCREEN_D1_PIN, SCREEN_D2_PIN, SCREEN_D3_PIN, SCREEN_D4_PIN, SCREEN_D5_PIN, SCREEN_D6_PIN, SCREEN_D7_PIN);

    _display = new Arduino_ST7789(_bus, SCREEN_RST_PIN, SCREEN_ROTATION, SCREEN_IPS, SCREEN_WIDTH, SCREEN_HEIGHT,
                              SCREEN_COL_OFFSET, SCREEN_ROW_OFFSET /* 1 */,
                              SCREEN_COL_OFFSET, SCREEN_ROW_OFFSET /* 2 */);

    _gfx = new Arduino_Canvas(SCREEN_WIDTH, SCREEN_HEIGHT, _display);
}

ST7789Display::~ST7789Display() {
    delete _gfx;
    delete _display;
    delete _bus;
}

void ST7789Display::begin() {
    pinMode(SCREEN_POWER, OUTPUT);
    digitalWrite(SCREEN_POWER, HIGH);
    
    ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);
    ledcAttachPin(GFX_BL, BACKLIGHT_PWM_CHANNEL);
    
    _gfx->begin();
    _gfx->setTextWrap(false);
}

void ST7789Display::setBacklight(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    uint32_t dutyCycle = (255 * percent) / 100;
    ledcWrite(BACKLIGHT_PWM_CHANNEL, dutyCycle);
}

void ST7789Display::setRotation(uint8_t rotation) {
    _gfx->setRotation(rotation);
}

int ST7789Display::width() {
    return _gfx->width();
}

int ST7789Display::height() {
    return _gfx->height();
}

void ST7789Display::fillScreen(uint16_t color) {
    _gfx->fillScreen(color);
}

void ST7789Display::drawRGBBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h) {
    _gfx->draw24bitRGBBitmap(x, y, bitmap, w, h);
}

void ST7789Display::setCursor(int16_t x, int16_t y) {
    _gfx->setCursor(x, y);
}

void ST7789Display::setTextColor(uint16_t color, uint16_t bg) {
    _gfx->setTextColor(color, bg);
}

void ST7789Display::setFont(DisplayFont font) {

    switch(font) {
        case TitleFont:
            _gfx->setFont(&futura_medium_bt16pt8b);
            break;
        case Heading1Font:
            _gfx->setFont(&futura_medium_bt14pt8b);
            break;
        case Heading2Font:
            _gfx->setFont(&futura_medium_bt12pt8b);
            break;
        case NormalFont:
        default:
            _gfx->setFont(&futura_medium_bt10pt8b);
            break;
    }
}

void ST7789Display::setTextSize(uint8_t size) {
    _gfx->setTextSize(size);
}

void ST7789Display::println(const String &s) {
    _gfx->println(s);
}

void ST7789Display::print(const String &s) {
    _gfx->print(s);
}

void ST7789Display::printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    _gfx->print(buffer);
}

int16_t ST7789Display::getCursorY() {
    return _gfx->getCursorY();
}

void ST7789Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _gfx->fillRect(x, y, w, h, color);
}

void ST7789Display::getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    _gfx->getTextBounds(str, x, y, x1, y1, w, h);
}

void ST7789Display::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    _gfx->drawCircle(x0, y0, r, color);
}

void ST7789Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    _gfx->drawLine(x0, y0, x1, y1, color);
}

void ST7789Display::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    _gfx->fillTriangle(x0, y0, x1, y1, x2, y2, color);
}

void ST7789Display::flush() {
    _gfx->flush();
}

#endif