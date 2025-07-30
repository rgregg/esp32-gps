#ifdef USE_SH1107_DISPLAY

#include "SH1107Display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "fonts/futura_medium_bt10pt8b.h"
#include "fonts/futura_medium_bt12pt8b.h"
#include "fonts/futura_medium_bt14pt8b.h"
#include "fonts/futura_medium_bt16pt8b.h"

#include <TLogPlus.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C  // May also be 0x3D depending on your display
#define SH1107_SDA 21
#define SH1107_SCL 22
#define SH1107_RST -1 // No reset pin for SH1107
#define SH1107_BL 38
#define SH1107_POWER 15

SH1107Display::SH1107Display() {
    _display = new Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
    TLogPlus::Log.println("SH1107: Display initalized.");
}

SH1107Display::~SH1107Display() {
    delete _display;
}

void SH1107Display::begin() {
    _display->begin();
    _display->setTextWrap(false);
}

void SH1107Display::setRotation(uint8_t rotation) {
    _display->setRotation(rotation);
}

int SH1107Display::width() {
    return _display->width();
}

int SH1107Display::height() {
    return _display->height();
}

void SH1107Display::fillScreen(uint16_t color) {
    _display->fillScreen(color);
}

void SH1107Display::drawRGBBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h) {
    uint8_t *grayBuf = (uint8_t*) malloc(w * h); 
    for (int i = 0; i < w * h; i++) {
        uint8_t r = bitmap[i * 3 + 0];
        uint8_t g = bitmap[i * 3 + 1];
        uint8_t b = bitmap[i * 3 + 2];

        // Convert to grayscale using luminance formula
        uint8_t gray = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
        grayBuf[i] = gray;
    }
    _display->drawGrayscaleBitmap(x, y, grayBuf, w, h);
}

void SH1107Display::setCursor(int16_t x, int16_t y) {
    _display->setCursor(x, y);
}

void SH1107Display::setTextColor(uint16_t color, uint16_t bg) {
    _display->setTextColor(color, bg);
}

void SH1107Display::setFont(DisplayFont font) {

    switch(font){
        case TitleFont:
            _display->setFont(&futura_medium_bt14pt8b);
            break;
        case Heading1Font:
            _display->setFont(&futura_medium_bt14pt8b);
            break;
        case Heading2Font:
            _display->setFont(&futura_medium_bt12pt8b);
            break;
        case NormalFont:
        default:
            _display->setFont(&futura_medium_bt10pt8b);
            break;
    }
}

void SH1107Display::setTextSize(uint8_t size) {
    _display->setTextSize(size);
}

void SH1107Display::println(const String &s) {
    _display->println(s);
}

void SH1107Display::print(const String &s) {
    _display->print(s);
}

void SH1107Display::printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    _display->print(buffer);
}

int16_t SH1107Display::getCursorY() {
    return _display->getCursorY();
}

void SH1107Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _display->fillRect(x, y, w, h, color);
}

void SH1107Display::getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    _display->getTextBounds(str, x, y, x1, y1, w, h);
}

void SH1107Display::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    _display->drawCircle(x0, y0, r, color);
}

void SH1107Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    _display->drawLine(x0, y0, x1, y1, color);
}

void SH1107Display::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    _display->fillTriangle(x0, y0, x1, y1, x2, y2, color);
}

void SH1107Display::flush() {
    _display->flush();
}

void SH1107Display::setBacklight(uint8_t percent) {
    // Monochrome display has no backlight
}

#endif