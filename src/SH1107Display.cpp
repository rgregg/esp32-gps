#include "SH1107Display.h"
#include "Constants.h"

SH1107Display::SH1107Display() {
    
    // Arduino_I2C
    // _bus = new Arduino_Wire(i2c_addr, cmdPrefix, dataPrefix, TwoWire);
    //     // SH1107_SDA, SH1107_SCL);

    // _display = new Arduino_SH1106(
    //     _bus, SH1107_RST, SH1107_ROTATION, true, SH1107_WIDTH, SH1107_HEIGHT, 0, 0, true);

    // _gfx = new Arduino_Canvas(SH1107_WIDTH, SH1107_HEIGHT, _display);
}

SH1107Display::~SH1107Display() {
    delete _gfx;
    delete _display;
    delete _bus;
}

void SH1107Display::begin() {
    _gfx->begin();
    _gfx->setTextWrap(false);
}

void SH1107Display::setRotation(uint8_t rotation) {
    _gfx->setRotation(rotation);
}

int SH1107Display::width() {
    return _gfx->width();
}

int SH1107Display::height() {
    return _gfx->height();
}

void SH1107Display::fillScreen(uint16_t color) {
    _gfx->fillScreen(color);
}

void SH1107Display::draw24bitRGBBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h) {
    _gfx->draw24bitRGBBitmap(x, y, bitmap, w, h);
}

void SH1107Display::setCursor(int16_t x, int16_t y) {
    _gfx->setCursor(x, y);
}

void SH1107Display::setTextColor(uint16_t color, uint16_t bg) {
    _gfx->setTextColor(color, bg);
}

void SH1107Display::setFont(const GFXfont *f) {
    _gfx->setFont(f);
}

void SH1107Display::setTextSize(uint8_t size) {
    _gfx->setTextSize(size);
}

void SH1107Display::println(const String &s) {
    _gfx->println(s);
}

void SH1107Display::print(const String &s) {
    _gfx->print(s);
}

void SH1107Display::printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    _gfx->print(buffer);
}

int16_t SH1107Display::getCursorY() {
    return _gfx->getCursorY();
}

void SH1107Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _gfx->fillRect(x, y, w, h, color);
}

void SH1107Display::getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    _gfx->getTextBounds(str, x, y, x1, y1, w, h);
}

void SH1107Display::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    _gfx->drawCircle(x0, y0, r, color);
}

void SH1107Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    _gfx->drawLine(x0, y0, x1, y1, color);
}

void SH1107Display::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    _gfx->fillTriangle(x0, y0, x1, y1, x2, y2, color);
}

void SH1107Display::flush() {
    _gfx->flush();
}