#include "ScreenManager.h"
#include <Arduino.h>
#include <TLogPlus.h>
#include <LittleFS.h>
#include "Constants.h"
#include <algorithm>


#define BACKLIGHT_PWM_CHANNEL 0
#define BACKLIGHT_PWM_FREQ 5000
#define BACKLIGHT_PWM_RESOLUTION 8
#define BG_COLOR 0
#define WHITE 255
#define RED 254
#define GREEN 253
#define YELLOW 252
#define BLUE 251
#define DARKCYAN 250
#define LIGHTGREY 249

#define ICON_BAR_WIDTH 32
#define LEFT_PADDING 10
#define TOP_PADDING 30
#define ICON_PADDING 10
#define ICON_SIZE 32


ScreenManager::ScreenManager(AppSettings *settings, Display* display) : 
    _settings(settings), _display(display), _orientation(LANDSCAPE), _screenMode(SCREEN_BOOT), _otaUpdateType("")
{
    _gpsManager = nullptr;
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

    _display->begin();
    
    int rotation = _settings->getInt(SETTING_DISPLAY_ROTATION, DISPLAY_ROTATION_DEFAULT);
    setRotation(rotation, false);
    refreshScreen(true);
}

void ScreenManager::setRotation(uint8_t rotation, bool redraw)
{
    _display->setRotation(rotation);
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
    // Force a full redraw of the screen
    _display->fillScreen(BG_COLOR);

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
            _display->setCursor(0, 20);
            _display->setTextColor(RED, BG_COLOR);
            _display->setFont(Heading1Font);
            _display->setTextSize(1);
            
            _display->println("This screen unintentionally left blank");
            _display->printf("[%u]", _screenMode);
            break;
    }
    _display->flush();
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
    int width = _display->width();
    int height = _display->height();

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
        const int bytes_per_pixel = 3;
        uint8_t *imgBuf = (uint8_t*) malloc(width * height * bytes_per_pixel);
        if (!imgBuf) return; // Check malloc success

        File file = LittleFS.open(filename, "r");
        if (!file || file.read(imgBuf, width * height * bytes_per_pixel) != width * height * bytes_per_pixel) {
            free(imgBuf);
            return; // Read error
        }

        // Cache it
        _bitmapCache[filename] = { imgBuf, width, height };
        cached = &_bitmapCache[filename];
    }

    _display->drawRGBBitmap(x, y, cached->data, cached->width, cached->height);
}

void ScreenManager::drawCoreScreen() 
{
    // Draw the date and time
    _display->setFont(Heading2Font);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->setCursor(LEFT_PADDING, TOP_PADDING);

    if (_gpsManager->getTimeStr().isEmpty() || _gpsManager->getDateStr().isEmpty()) {
        _display->println("No date/time yet");
    } else {
        _display->print(_gpsManager->getDateStr());
        _display->print(" ");
        _display->println(_gpsManager->getTimeStr());
    }

    // Fill the space with black where we're going to write the GPS data
    int cursorY = _display->getCursorY();
    _display->fillRect(0, cursorY,
                   _display->width() - ICON_BAR_WIDTH - ICON_PADDING, _display->height() - cursorY,
                   BG_COLOR);

    // Draw the lat/long
    if(_gpsManager->hasFix())
    {
        DMS latitude = _gpsManager->getLatitude();
        _display->setCursor(LEFT_PADDING, cursorY);
        this->drawDMS(latitude);
        _display->println("");

        DMS longitude = _gpsManager->getLongitude();
        moveCursorX(LEFT_PADDING);
        this->drawDMS(longitude);
    }
    else
    {
        _display->setCursor(LEFT_PADDING, cursorY);
        _display->setTextColor(YELLOW, BG_COLOR);
        _display->println("Waiting for GPS fix");
        moveCursorX(LEFT_PADDING);
        _display->setTextColor(WHITE, BG_COLOR);
        _display->setFont(NormalFont);
        _display->println("Check GPS receiver antenna");
    }
}

void ScreenManager::drawDMS(DMS value) {
    _display->print(String(value.direction));
    _display->print(" ");
    _display->print(String(value.degrees));
    _display->print("\xB0");
    _display->print(String(value.minutes));
    _display->print("\'");
    _display->print(String(value.seconds, 2));
    _display->print("\"");
}

void ScreenManager::drawNavigationScreen()
{
    _display->setTextColor(WHITE, BG_COLOR);

    _display->setFont(Heading1Font);
    _display->setTextSize(1); 

    bool hasFix = _gpsManager->hasFix();
    
    int angle = _gpsManager->getDirectionFromTrueNorth();
    if (!hasFix) angle = 0;
    // Draw the compass
    drawCompass(44, 60, 40, angle);

    int speed_x = 190, speed_y = 56;
    int16_t x1, y1;
    uint16_t w, h;
    String speed = String(_gpsManager->getSpeed(), 1);
    if (!hasFix) speed = "No Fix";
    _display->getTextBounds(speed, 0, 0, &x1, &y1, &w, &h);
    _display->setCursor(speed_x - (w/2), speed_y);
    _display->println(speed);

    if (hasFix)
    {
        this->setFontAndSize(NormalFont, 1);
        String units = "knots";
        _display->getTextBounds(units, 0, 0, &x1, &y1, &w, &h);
        moveCursorX(speed_x - (w/2));
        _display->print(units);
    }
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

    _display->drawCircle(centerX, centerY, radius, OUTER_COLOR);
    _display->drawLine(centerX, centerY - radius, centerX, centerY + radius, INNER_COLOR); // N-S
    _display->drawLine(centerX - radius, centerY, centerX + radius, centerY, INNER_COLOR); // W-E

    _display->fillTriangle(
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
        _display->drawLine(x1, y1, x2, y2, TICK_MARK_COLOR);
    }

    // Print the N for north
    
    _display->setTextSize(1);
    _display->setFont(NormalFont);        // SMALL_TEXT_FONT?

    String north = "N";

    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(north, 0, 0, &x1, &y1, &w, &h);
    _display->setCursor(centerX - (w/2) - 3, centerY - radius - 18);
    _display->print("N");

    // Print the course direction
    String course = String(headingDegrees) + "\xB0";
    setFontAndSize(Heading1Font, 1);
    _display->getTextBounds(course, 0, 0, &x1, &y1, &w, &h);
    _display->setCursor(centerX - (w/2), centerY + radius + h + 10);
    _display->print(course);

    // Rotate by -90 so north is the right direction
    float headingRad = (headingDegrees - 90) * DEG_TO_RAD;
    int needleLength = radius - 10;
    int xTip = centerX + cos(headingRad) * needleLength;
    int yTip = centerY + sin(headingRad) * needleLength;

    _display->drawLine(centerX, centerY, xTip, yTip, ARROW_COLOR);
    
}

void ScreenManager::drawAboutScreen() 
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    setFontAndSize(TitleFont, 1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Nomaduino GPS");

    _display->setCursor(122, 70);
    _display->setFont(NormalFont);
    _display->println("ESP32 GPS Receiver");
    moveCursorX(122);
    _display->println("Version");
    moveCursorX(122);
    _display->println(AUTO_VERSION);

    drawIcon(8, 45, 92, 101, "/images/nomaduino-92x101.rgb");
}

void ScreenManager::drawWiFiScreen()
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->setTextColor(WHITE, BG_COLOR);
    setFontAndSize(Heading1Font, 1);
    _display->println("WIFI Information");
    
    setFontAndSize(NormalFont, 1);
    moveCursorX(LEFT_PADDING);
    _display->println(WiFi.SSID());
    moveCursorX(LEFT_PADDING);
    _display->println(currentWiFiStatus());
    moveCursorX(LEFT_PADDING);
    _display->println("IP: " + WiFi.localIP().toString());
    moveCursorX(LEFT_PADDING);
    _display->println("RSSI: " + String(WiFi.RSSI()));
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

    _display->setCursor(140, 54);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->setFont(TitleFont);
    _display->setTextSize(1);
    _display->println("Nomaduino");

    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->setCursor(140, _display->getCursorY());
    _display->println(AUTO_VERSION);
}

void ScreenManager::drawGPSScreen()
{
    _display->setTextColor(WHITE, BG_COLOR);
    _display->setFont(Heading1Font);
    _display->setTextSize(1);
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->println("GPS Information");

    _display->setFont(NormalFont);
    _display->setTextSize(1);
    
    // Fix
    _display->setTextColor(_gpsManager->hasFix() ? GREEN : RED, BG_COLOR);
    moveCursorX(LEFT_PADDING);
    _display->println(_gpsManager->getFixStr());
    
    // Satellites
    _display->setTextColor(WHITE, BG_COLOR);
    moveCursorX(LEFT_PADDING);
    _display->println(_gpsManager->getSatellitesStr());
    
    // Antenna
    moveCursorX(LEFT_PADDING);
    _display->println(_gpsManager->getAntennaStr());
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

void ScreenManager::setOTAStatus(String updateType, uint8_t percentComplete)
{
    _otaStatusPercentComplete = percentComplete;
    _otaUpdateType = updateType;
    refreshScreen();
}

void ScreenManager::setPortalSSID(String ssid)
{
    _portalSSID = ssid;
    refreshScreen();
}

void ScreenManager::drawUpdateScreen()
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->setFont(TitleFont);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Nomaduino");

    _display->setCursor(LEFT_PADDING, 60);
    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->print(_otaUpdateType);
    _display->print(" updating... ");
    _display->print(String(_otaStatusPercentComplete));
    _display->print("%");
}

void ScreenManager::drawWiFiPortalScreen()
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    setFontAndSize(TitleFont, 1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Nomaduino GPS");

    setFontAndSize(NormalFont, 1);
    moveCursorX(LEFT_PADDING);
    _display->setTextColor(YELLOW, BG_COLOR);
    _display->println("Configure via WiFi");

    moveCursorX(LEFT_PADDING);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->print("SSID: ");
    _display->println(_portalSSID);

    moveCursorX(LEFT_PADDING);
    _display->print("http://");
    _display->println(WiFi.softAPIP().toString());
}

void ScreenManager::moveCursorX(int x)
{
    _display->setCursor(x, _display->getCursorY());
}

void ScreenManager::setFontAndSize(DisplayFont font, int size)
{
    _display->setFont(font);
    _display->setTextSize(size);
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


    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->setFont(TitleFont);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Debug");

    _display->setCursor(LEFT_PADDING, 60);
    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->printf("HEAP: %s / %s\n", humanReadableBytes(ESP.getFreeHeap()), humanReadableBytes(ESP.getHeapSize()));
    moveCursorX(LEFT_PADDING);
    _display->printf("PSRAM: %s / %s\n", humanReadableBytes(ESP.getFreePsram()), humanReadableBytes(ESP.getPsramSize()));
}

