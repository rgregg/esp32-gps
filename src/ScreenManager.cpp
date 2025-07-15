#include "ScreenManager.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TLogPlus.h>
#include <LittleFS.h>
#include "Constants.h"
#include <algorithm>

#include "fonts/futura_medium_bt10pt8b.h"
#include "fonts/futura_medium_bt12pt8b.h"
#include "fonts/futura_medium_bt14pt8b.h"
#include "fonts/futura_medium_bt16pt8b.h"

#define BACKLIGHT_PWM_CHANNEL 0
#define BACKLIGHT_PWM_FREQ 5000
#define BACKLIGHT_PWM_RESOLUTION 8
#define BG_COLOR BLACK

#define TITLE_FONT futura_medium_bt16pt8b
#define TITLE_FONT_HEIGHT 20
#define HEADING_FONT futura_medium_bt14pt8b
#define HEADING_FONT_HEIGHT 16
#define HEADING_2_FONT futura_medium_bt12pt8b
#define HEADING_2_FONT_HEIGHT 14
#define NORMAL_FONT futura_medium_bt10pt8b
#define NORMAL_FONT_HEIGHT 12
#define ICON_BAR_WIDTH 32
#define LEFT_PADDING 10
#define TOP_PADDING 30
#define ICON_PADDING 10
#define ICON_SIZE 32


ScreenManager::ScreenManager(AppSettings *settings) : 
    _settings(settings), _orientation(LANDSCAPE), _screenMode(SCREEN_BOOT)
{
    _gpsManager = nullptr;
    _bus = new Arduino_ESP32PAR8Q(
        SCREEN_DC_PIN, SCREEN_CS_PIN, SCREEN_WR_PIN, SCREEN_RD_PIN,
        SCREEN_D0_PIN, SCREEN_D1_PIN, SCREEN_D2_PIN, SCREEN_D3_PIN, SCREEN_D4_PIN, SCREEN_D5_PIN, SCREEN_D6_PIN, SCREEN_D7_PIN);

    _display = new Arduino_ST7789(_bus, SCREEN_RST_PIN, SCREEN_ROTATION, SCREEN_IPS, SCREEN_WIDTH, SCREEN_HEIGHT,
                              SCREEN_COL_OFFSET, SCREEN_ROW_OFFSET /* 1 */,
                              SCREEN_COL_OFFSET, SCREEN_ROW_OFFSET /* 2 */);

    _gfx = new Arduino_Canvas(SCREEN_WIDTH, SCREEN_HEIGHT, _display);

    _refreshGPSTime = _settings->getInt(SETTING_SCREEN_REFRESH_INTERVAL, SCREEN_REFRESH_INTERVAL_DEFAULT);
    _refreshOtherTime = _settings->getInt(SETTING_REFRESH_INTERVAL_OTHER, REFRESH_INTERVAL_OTHER_DEFAULT);
}

void ScreenManager::begin()
{
    _refreshTimer = millis();

    // Power on the screen
    pinMode(SCREEN_POWER, OUTPUT);
    digitalWrite(SCREEN_POWER, HIGH);

    // Configure PWN backlight control
    ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);
    ledcAttachPin(GFX_BL, BACKLIGHT_PWM_CHANNEL);
    setBacklight(_settings->getInt("backlight", 100));

    _gfx->begin();
    _gfx->setTextWrap(false);
    
    int rotation = _settings->getInt(SETTING_DISPLAY_ROTATION, DISPLAY_ROTATION_DEFAULT);
    setRotation(rotation, false);
    refreshScreen(true);
}

void ScreenManager::setRotation(uint8_t rotation, bool redraw)
{
    _gfx->setRotation(rotation);
    if (rotation == 0 || rotation == 2) {
        _orientation = PORTRAIT;
    } else {
        _orientation = LANDSCAPE;
    }
    if (redraw)
    {
        refreshScreen(true);
    }
}

void ScreenManager::loop()
{
    refreshIfTimerElapsed(200);
}

void ScreenManager::setBacklight(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    uint32_t dutyCycle = (255 * percent) / 100;
    ledcWrite(BACKLIGHT_PWM_CHANNEL, dutyCycle);
    _settings->setInt("backlight", percent);
}

void ScreenManager::setGPSManager(GPSManager *manager)
{
    _gpsManager = manager;
}

bool ScreenManager::refreshIfTimerElapsed(uint32_t maxTime)
{
    if (millis() - _refreshTimer > maxTime)
    {
        _refreshTimer = millis(); // reset the timer
        refreshScreen();
        return true;
    }
    return false;
}

void ScreenManager::setScreenMode(ScreenMode mode)
{
    if (mode < 0 || mode >= SCREEN_MAX) {
        TLogPlus::Log.warningln("ScreenManager tried to move to a screen outside of the range.");
        return;
    }

    TLogPlus::Log.printf("ScreenManager: setScreenMode to %u\n", mode);
    if (_screenMode != mode)
    {
        _screenMode = mode;
        refreshScreen(true);
    }
}

bool ScreenManager::isScreenMode(ScreenMode compareMode)
{
    return _screenMode == compareMode;
}

ScreenMode ScreenManager::getScreenMode()
{
    return _screenMode;
}

void ScreenManager::refreshScreen(bool fullRefresh)
{
    _gfx->startWrite();
    
    // Force a full redraw of the screen
    _gfx->fillScreen(BG_COLOR);

    if (_screenMode != SCREEN_BOOT && _screenMode != SCREEN_ABOUT)
    {
        drawIconBar();
    }

    switch (_screenMode) {
        case SCREEN_BOOT:
            drawBootScreen();
            break;
        case SCREEN_ABOUT:
            drawAboutScreen();
            break;
        case SCREEN_CORE:
            drawCoreScreen();
            break;
        case SCREEN_NAVIGATION:
            drawNavigationScreen();
            break;
        case SCREEN_WIFI:
            drawWiFiScreen();
            break;
        case SCREEN_GPS:
            drawGPSScreen();
            break;
        case SCREEN_UPDATE_OTA:
            drawUpdateScreen();
            break;
        case SCREEN_NEEDS_CONFIG:
            drawWiFiPortalScreen();
            break;
        case SCREEN_DEVICE_DEBUG:
            drawDebugScreen();
            break;
        default:
            _gfx->setCursor(0, 20);
            _gfx->setTextColor(RED, BG_COLOR);
            _gfx->setFont(&HEADING_FONT);
            _gfx->setTextSize(1);
            
            _gfx->println("This screen unintentionally left blank");
            _gfx->printf("[%u]", _screenMode);
            break;
    }
    _gfx->endWrite();
    _gfx->flush();
}

static String imagePathForWiFiStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        int signal = WiFi.RSSI();
        if (signal <= -80) {
            return "/images/wifi-32-low.rgb";
        } else if (signal <= -67) {
            return "/images/wifi-32-medium.rgb";
        } else {
            return "/images/wifi-32-high.rgb";
        }
    } else {
        return "/images/wifi-32-disconnected.rgb";
    }
}

static String imagePathForBatteryStatus() {
    // TODO: Implement battery support more broadly
    return "/images/battery-32-none.rgb";
}

/// @brief Draws the status icon bar on the screen - either on the right side in landscape or at the bottom in portrait
void ScreenManager::drawIconBar()
{
    int width = _gfx->width();
    int height = _gfx->height();

    const int iconDimension_x = ICON_SIZE;
    const int iconDimension_y = ICON_SIZE;
    const int iconPadding = ICON_PADDING;

    int pos_x, pos_y;
    std::function<void(void)> incrementPosition;
    if (_orientation == LANDSCAPE)
    {
        pos_x = width - iconPadding - iconDimension_x;
        pos_y = iconPadding;
        incrementPosition = [&pos_y]() { pos_y += iconPadding + iconDimension_y; };
    } else {
        pos_x = iconPadding;
        pos_y = height - iconPadding - iconDimension_y;
        incrementPosition = [&pos_x]() { pos_x += iconPadding + iconDimension_x; };
    }

    // Draw the WiFi icon
    drawIcon(pos_x, pos_y, iconDimension_x, iconDimension_y, imagePathForWiFiStatus());
    incrementPosition();

    // Draw the GPS icon
    String gpsImagePath = _gpsManager->hasFix() ? "/images/gps-32-connected.rgb" : "/images/gps-32-disconnected.rgb";
    drawIcon(pos_x, pos_y, iconDimension_x, iconDimension_y, gpsImagePath);
    incrementPosition();

    // Draw the battery icon
    drawIcon(pos_x, pos_y, iconDimension_x, iconDimension_y, imagePathForBatteryStatus());
    incrementPosition();
}

void ScreenManager::drawIcon(int x, int y, int width, int height, String filename)
{
    CachedBitmap *cached = nullptr;

    // Check if image is cached
    auto it = _bitmapCache.find(filename);
    if (it != _bitmapCache.end()) {
        cached = &it->second;
    } else {
        // Allocate and load image from FS
        uint8_t *imgBuf = (uint8_t*) malloc(width * height * 3);
        if (!imgBuf) return; // Check malloc success

        File file = LittleFS.open(filename, "r");
        if (!file || file.read(imgBuf, width * height * 3) != width * height * 3) {
            free(imgBuf);
            return; // Read error
        }

        // Cache it
        _bitmapCache[filename] = { imgBuf, width, height };
        cached = &_bitmapCache[filename];
    }

    _gfx->draw24bitRGBBitmap(x, y, cached->data, cached->width, cached->height);
}

void ScreenManager::drawCoreScreen() 
{
    // Draw the date and time
    _gfx->setFont(&HEADING_2_FONT);
    _gfx->setTextSize(1);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->setCursor(LEFT_PADDING, TOP_PADDING);

    if (_gpsManager->getTimeStr().isEmpty() || _gpsManager->getDateStr().isEmpty()) {
        _gfx->println("No date/time yet");
    } else {
        _gfx->print(_gpsManager->getDateStr());
        _gfx->print(" ");
        _gfx->println(_gpsManager->getTimeStr());
    }

    // Fill the space with black where we're going to write the GPS data
    int cursorY = _gfx->getCursorY();
    _gfx->fillRect(0, cursorY,
                   _gfx->width() - ICON_BAR_WIDTH - ICON_PADDING, _gfx->height() - cursorY,
                   BG_COLOR);

    // Draw the lat/long
    if(_gpsManager->hasFix())
    {
        DMS latitude = _gpsManager->getLatitude();
        _gfx->setCursor(LEFT_PADDING, cursorY);
        drawDMS(latitude);
        _gfx->println();

        DMS longitude = _gpsManager->getLongitude();
        moveCursorX(LEFT_PADDING);
        drawDMS(longitude);
    }
    else
    {
        _gfx->setCursor(LEFT_PADDING, cursorY);
        _gfx->setTextColor(YELLOW, BG_COLOR);
        _gfx->println("Waiting for GPS fix");
        moveCursorX(LEFT_PADDING);
        _gfx->setTextColor(WHITE, BG_COLOR);
        _gfx->setFont(&NORMAL_FONT);
        _gfx->println("Check GPS receiver antenna");
    }
}

/// @brief Draws to the graphics device the direction, degrees, minutes, and seconds at the current cursor position.
void ScreenManager::drawDMS(DMS value) {
    _gfx->print(String(value.direction));
    _gfx->print(" ");
    _gfx->print(String(value.degrees));
    _gfx->print("\xB0");
    _gfx->print(String(value.minutes));
    _gfx->print("\'");
    _gfx->print(String(value.seconds, 2));
    _gfx->print("\"");
}

void ScreenManager::drawNavigationScreen()
{
    _gfx->setTextColor(WHITE, BG_COLOR);
    
    
    _gfx->setFont(&HEADING_FONT);
    _gfx->setTextSize(1); 

    
    int angle = _gpsManager->getDirectionFromTrueNorth();
    // Draw the compass
    drawCompass(44, 60, 40, angle);

    int speed_x = 190, speed_y = 56;
    int16_t x1, y1;
    uint16_t w, h;
    String speed = String(_gpsManager->getSpeed(), 1);
    _gfx->getTextBounds(speed, 0, 0, &x1, &y1, &w, &h);
    _gfx->setCursor(speed_x - (w/2), speed_y);
    _gfx->println(speed);

    setFontAndSize(&NORMAL_FONT, 1);
    String units = "knots";
    _gfx->getTextBounds(units, 0, 0, &x1, &y1, &w, &h);
    moveCursorX(speed_x - (w/2));
    _gfx->print(units);

    
}

/// @brief Draws a compass rose with the upper left corner at pos_x, pos_y and of dimensions width, with an arrow pointing in direction.
/// @param pos_x 
/// @param pos_y 
/// @param width 
/// @param direction 
void ScreenManager::drawCompass(int pos_x, int pos_y, int radius, int headingDegrees)
{
    uint16_t OUTER_COLOR = BLUE;
    uint16_t INNER_COLOR = DARKCYAN;
    uint16_t TICK_MARK_COLOR = LIGHTGREY;
    uint16_t TEXT_COLOR = WHITE;
    uint16_t ARROW_COLOR = RED;

    int centerX = pos_x + radius/2;
    int centerY = pos_y + radius/2;

    _gfx->drawCircle(centerX, centerY, radius, OUTER_COLOR);
    _gfx->drawLine(centerX, centerY - radius, centerX, centerY + radius, INNER_COLOR); // N-S
    _gfx->drawLine(centerX - radius, centerY, centerX + radius, centerY, INNER_COLOR); // W-E

    _gfx->fillTriangle(
        centerX, centerY - radius - 10,        // tip of the arrow
        centerX - 5, centerY - radius + 5,     // bottom left
        centerX + 5, centerY - radius + 5,     // bottom right
        ARROW_COLOR
    );

    for (int angle = 0; angle < 360; angle += 30) {
        float rad = angle * DEG_TO_RAD;
        int x1 = centerX + cos(rad) * (radius - 5);
        int y1 = centerY + sin(rad) * (radius - 5);
        int x2 = centerX + cos(rad) * radius;
        int y2 = centerY + sin(rad) * radius;
        _gfx->drawLine(x1, y1, x2, y2, TICK_MARK_COLOR);
    }

    // Print the N for north
    
    _gfx->setTextSize(1);
    _gfx->setFont(&NORMAL_FONT);        // SMALL_TEXT_FONT?

    String north = "N";

    int16_t x1, y1;
    uint16_t w, h;
    _gfx->getTextBounds(north, 0, 0, &x1, &y1, &w, &h);
    _gfx->setCursor(centerX - (w/2) - 3, centerY - radius - 18);
    _gfx->print("N");

    // Print the course direction
    String course = String(headingDegrees) + "\xB0";
    setFontAndSize(&HEADING_FONT, 1);
    _gfx->getTextBounds(course, 0, 0, &x1, &y1, &w, &h);
    _gfx->setCursor(centerX - (w/2), centerY + radius + h + 10);
    _gfx->print(course);

    // Rotate by -90 so north is the right direction
    float headingRad = (headingDegrees - 90) * DEG_TO_RAD;
    int needleLength = radius - 10;
    int xTip = centerX + cos(headingRad) * needleLength;
    int yTip = centerY + sin(headingRad) * needleLength;

    _gfx->drawLine(centerX, centerY, xTip, yTip, ARROW_COLOR);
    
}

void ScreenManager::drawAboutScreen() 
{
    _gfx->setCursor(LEFT_PADDING, TOP_PADDING);
    setFontAndSize(&TITLE_FONT, 1);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->println("Nomadiuno GPS");

    _gfx->setCursor(122, 70);
    _gfx->setFont(&NORMAL_FONT);
    _gfx->println("ESP32 GPS Receiver");
    moveCursorX(122);
    _gfx->println("Version");
    moveCursorX(122);
    _gfx->println(AUTO_VERSION);

    drawIcon(8, 45, 92, 101, "/images/nomaduino-92x101.rgb");
}

void ScreenManager::drawWiFiScreen()
{
    _gfx->setCursor(LEFT_PADDING, TOP_PADDING);
    _gfx->setTextColor(WHITE, BG_COLOR);
    setFontAndSize(&HEADING_FONT, 1);
    _gfx->println("WIFI Information");
    
    setFontAndSize(&NORMAL_FONT, 1);
    moveCursorX(LEFT_PADDING);
    _gfx->println(WiFi.SSID());
    moveCursorX(LEFT_PADDING);
    _gfx->println(currentWiFiStatus());
    moveCursorX(LEFT_PADDING);
    _gfx->println("IP: " + WiFi.localIP().toString());
    moveCursorX(LEFT_PADDING);
    _gfx->println("RSSI: " + String(WiFi.RSSI()));
}

void ScreenManager::showDefaultScreen()
{
    setScreenMode(_screenLoop[0]);
}

void ScreenManager::moveNextScreen(int8_t direction)
{
    constexpr int SCREEN_LOOP_SIZE = sizeof(_screenLoop) / sizeof(_screenLoop[0]);
    // Get the current index
    ScreenMode current = getScreenMode();
    int currentIndex = -1;
    for (int i = 0; i < SCREEN_LOOP_SIZE; ++i) {
        if (_screenLoop[i] == current) {
            currentIndex = i;
            break;
        }
    }

    if (currentIndex == -1) {
        // If the current screen mode isn't found, default to first
        setScreenMode(_screenLoop[0]);
        return;
    }

    // Calculate new index with wrap-around
    int newIndex = (currentIndex + direction + SCREEN_LOOP_SIZE) % SCREEN_LOOP_SIZE;
    setScreenMode(_screenLoop[newIndex]);
}

void ScreenManager::drawBootScreen()
{
    int pos_x = 17, pos_y = 22;
    drawIcon(pos_x, pos_y, 122, 122, "/images/nomaduino-122.rgb");

    _gfx->setCursor(140, 54);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->setFont(&TITLE_FONT);
    _gfx->setTextSize(1);
    _gfx->println("Nomadiuno");

    _gfx->setFont(&NORMAL_FONT);
    _gfx->setTextSize(1);
    _gfx->setCursor(140, _gfx->getCursorY());
    _gfx->println(AUTO_VERSION);
}

void ScreenManager::drawGPSScreen()
{
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->setFont(&HEADING_FONT);
    _gfx->setTextSize(1);
    _gfx->setCursor(LEFT_PADDING, TOP_PADDING);
    _gfx->println("GPS Information");

    _gfx->setFont(&NORMAL_FONT);
    _gfx->setTextSize(1);
    
    // Fix
    _gfx->setTextColor(_gpsManager->hasFix() ? GREEN : RED, BG_COLOR);
    moveCursorX(LEFT_PADDING);
    _gfx->println(_gpsManager->getFixStr());
    
    // Satellites
    _gfx->setTextColor(WHITE, BG_COLOR);
    moveCursorX(LEFT_PADDING);
    _gfx->println(_gpsManager->getSatellitesStr());
    
    // Antenna
    moveCursorX(LEFT_PADDING);
    _gfx->println(_gpsManager->getAntennaStr());
}

const char* ScreenManager::currentWiFiStatus()
{
  int status = WiFi.status();
  switch(status) {
    case WL_NO_SSID_AVAIL:
        return "Network Not Available";
    case WL_CONNECT_FAILED:
        return "Connection Failed";
    case WL_CONNECTION_LOST:
        return "Connection Lost";
    case WL_CONNECTED:
        return "Connected";
    case WL_DISCONNECTED:
        return "Disconnected";
    default:
        return "Searching";
  }
  return "N/A";
}

void ScreenManager::setOTAStatus(uint8_t percentComplete)
{
    _otaStatusPercentComplete = percentComplete;
    refreshScreen();
}

void ScreenManager::setPortalSSID(String ssid)
{
    _portalSSID = ssid;
    refreshScreen();
}

void ScreenManager::drawUpdateScreen()
{
    _gfx->setCursor(LEFT_PADDING, TOP_PADDING);
    _gfx->setFont(&TITLE_FONT);
    _gfx->setTextSize(1);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->println("Nomadiuno");

    _gfx->setCursor(LEFT_PADDING, 60);
    _gfx->setFont(&NORMAL_FONT);
    _gfx->setTextSize(1);
    _gfx->print("Updating... ");
    _gfx->print(_otaStatusPercentComplete);
    _gfx->print("%");
}

void ScreenManager::drawWiFiPortalScreen()
{
    _gfx->setCursor(LEFT_PADDING, TOP_PADDING);
    setFontAndSize(&TITLE_FONT, 1);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->println("Nomadiuno GPS");

    setFontAndSize(&NORMAL_FONT, 1);
    moveCursorX(LEFT_PADDING);
    _gfx->setTextColor(YELLOW, BG_COLOR);
    _gfx->println("Configure via WiFi");

    moveCursorX(LEFT_PADDING);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->print("SSID: ");
    _gfx->println(_portalSSID);

    moveCursorX(LEFT_PADDING);
    _gfx->print("http://");
    _gfx->println(WiFi.softAPIP().toString());
}

void ScreenManager::moveCursorX(int x)
{
    _gfx->setCursor(x, _gfx->getCursorY());
}

void ScreenManager::setFontAndSize(const GFXfont *f, int size)
{
    _gfx->setFont(f);
    _gfx->setTextSize(size);
}

void ScreenManager::drawDebugScreen()
{
    auto humanReadableBytes = [](uint16_t bytes) -> String {
        if (bytes < 1024) {
            return String(bytes) + " b";
        }
        if (bytes < 1024 * 1024) {
            return String(bytes / 1024.0, 1) + " kb";
        }
        return String(bytes / 1048576.0, 1) + " mb";
    };


    _gfx->setCursor(LEFT_PADDING, TOP_PADDING);
    _gfx->setFont(&TITLE_FONT);
    _gfx->setTextSize(1);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->println("Debug");

    _gfx->setCursor(LEFT_PADDING, 60);
    _gfx->setFont(&NORMAL_FONT);
    _gfx->setTextSize(1);
    _gfx->printf("HEAP: %s / %s\n", humanReadableBytes(ESP.getFreeHeap()), humanReadableBytes(ESP.getHeapSize()));
    moveCursorX(LEFT_PADDING);
    _gfx->printf("PSRAM: %s / %s\n", humanReadableBytes(ESP.getFreePsram()), humanReadableBytes(ESP.getPsramSize()));
}

