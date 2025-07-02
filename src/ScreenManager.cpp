#include "ScreenManager.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TLogPlus.h>
#include <LittleFS.h>
#include "Constants.h"
#include <algorithm>

#define BACKLIGHT_PWM_CHANNEL 0
#define BACKLIGHT_PWM_FREQ 5000
#define BACKLIGHT_PWM_RESOLUTION 8
#define BG_COLOR BLACK

ScreenManager::ScreenManager(AppSettings *settings) : 
    _settings(settings)
{
    _gpsManager = nullptr;
    _screenMode = ScreenMode_BOOT;

    _bus = new Arduino_ESP32PAR8Q(
        SCREEN_DC_PIN, SCREEN_CS_PIN, SCREEN_WR_PIN, SCREEN_RD_PIN,
        SCREEN_D0_PIN, SCREEN_D1_PIN, SCREEN_D2_PIN, SCREEN_D3_PIN, SCREEN_D4_PIN, SCREEN_D5_PIN, SCREEN_D6_PIN, SCREEN_D7_PIN);

    _gfx = new Arduino_ST7789(_bus, SCREEN_RST_PIN, SCREEN_ROTATION, SCREEN_IPS, SCREEN_WIDTH, SCREEN_HEIGHT,
                              SCREEN_COL_OFFSET, SCREEN_ROW_OFFSET /* 1 */,
                              SCREEN_COL_OFFSET, SCREEN_ROW_OFFSET /* 2 */);

    _refreshGPSTime = _settings->getInt(SETTING_SCREEN_REFRESH_INTERVAL, SCREEN_REFRESH_INTERVAL_DEFAULT);
    _refreshOtherTime = _settings->getInt(SETTING_REFRESH_INTERVAL_OTHER, 5000);
}

void ScreenManager::begin()
{
    _refreshTimer = millis();

    // Power on the screen
    pinMode(SCREEN_POWER, OUTPUT);
    digitalWrite(SCREEN_POWER, HIGH);

    ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);
    ledcAttachPin(GFX_BL, BACKLIGHT_PWM_CHANNEL);

    setBacklight(_settings->getInt("backlight", 100));

    _gfx->begin();
    _gfx->setRotation(_settings->getInt(SETTING_DISPLAY_ROTATION, DISPLAY_ROTATION_DEFAULT));

    refreshScreen(true);
}

void ScreenManager::setRotation(uint8_t rotation)
{
    _gfx->setRotation(rotation);
    refreshScreen(true);
}

void ScreenManager::loop()
{
    switch (_screenMode)
    {
    case ScreenMode_BOOT:
    case ScreenMode_PORTAL:
    case ScreenMode_ABOUT:
        // No refresh necessary
        break;
    case ScreenMode_WIFI:
    case ScreenMode_OTA:
        refreshIfTimerElapsed(_refreshOtherTime);
        break;
    case ScreenMode_GPS:
    case ScreenMode_ADVANCED:
        refreshIfTimerElapsed(_refreshGPSTime);
        break;
    default:
        TLogPlus::Log.debugln("Unhandled screen mode in loop");
        refreshIfTimerElapsed(2000);
    }
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
    if (fullRefresh) {
        _gfx->fillScreen(BG_COLOR);
    }
    switch (_screenMode) {
        case ScreenMode_BOOT:
            drawBootScreen(false);
            break;
        case ScreenMode_ABOUT:
            drawBootScreen(true);
            break;
        case ScreenMode_ADVANCED:
            drawGPSScreen(false);
            break;
        case ScreenMode_WIFI:
            drawWiFiScreen();
            break;
        case ScreenMode_GPS:
            drawGPSScreen(true);
            break;
        case ScreenMode_OTA:
            _gfx->setCursor(0, 0);
            _gfx->setTextColor(WHITE, BG_COLOR);
            _gfx->setTextSize(3);
            _gfx->println("ESP32-GPS");
            _gfx->setTextSize(2);
            _gfx->setTextColor(YELLOW, BG_COLOR);
            _gfx->println("Update installing");
            _gfx->setTextSize(3);
            _gfx->print(_otaStatusPercentComplete);
            _gfx->print("%");
            break;
        case ScreenMode_PORTAL:
            _gfx->setCursor(0, 0);
            _gfx->setTextColor(RED, BG_COLOR);
            _gfx->setTextSize(3);
            _gfx->println("ESP32-GPS Needs Configuration");
            _gfx->setTextColor(WHITE, BG_COLOR);
            _gfx->print("SSID: ");
            _gfx->println(_portalSSID);
            _gfx->print("http://");
            _gfx->println(WiFi.softAPIP().toString());
            break;
        default:
            _gfx->setCursor(0, 0);
            _gfx->setTextColor(RED, BG_COLOR);
            _gfx->setTextSize(3);
            _gfx->printf("Unhandled Screen: %d", _screenMode);
            break;
    }
    _gfx->endWrite();
}

void ScreenManager::drawWiFiScreen()
{
    _gfx->setCursor(0, 0);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->println("ESP32-GPS");
    _gfx->print("WiFi ");
    _gfx->println(currentWiFiStatus());
    _gfx->println("IP: " + WiFi.localIP().toString());
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

void ScreenManager::drawBootScreen(bool showAbout)
{
    _gfx->setCursor(5, 5);
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->setTextSize(3);
    if (showAbout)
        _gfx->print("ABOUT ");
    _gfx->print("ESP32 GPS");
    _gfx->setTextSize(2);
    _gfx->setCursor(5, 30);
    _gfx->printf("Version: %s", AUTO_VERSION);
    if (showAbout)
        _gfx->print("\ngithub.com/rgregg/esp32-gps");

    int IMG_WIDTH = 64;
    int IMG_HEIGHT = IMG_WIDTH;

    int screenWidth = _gfx->width();
    int screenHeight = _gfx->height();
    int bmp_x = (screenWidth - IMG_WIDTH) / 2;
    int bmp_y = (screenHeight - IMG_HEIGHT) / 2;

    uint8_t *imgBuf = (uint8_t*) malloc(IMG_WIDTH * IMG_HEIGHT * 3);
    File file = LittleFS.open("/images/gps.rgb", "r");
    file.read(imgBuf, IMG_WIDTH * IMG_HEIGHT * 3);
    _gfx->draw24bitRGBBitmap(bmp_x, bmp_y + 10, imgBuf, IMG_WIDTH, IMG_HEIGHT);
    free(imgBuf);
}

void ScreenManager::drawGPSScreen(bool simple)
{
    _gfx->setTextColor(WHITE, BG_COLOR);
    _gfx->setTextSize(simple ? 3 : 2);
    _gfx->setCursor(0, 0);
    _gfx->println("ESP32-GPS");
    _gfx->print(_gpsManager->getDateStr());
    _gfx->print(" ");
    _gfx->println(_gpsManager->getTimeStr());

    if (_gpsManager->hasFix())
    {
        _gfx->setTextColor(GREEN, BG_COLOR);
        _gfx->println(_gpsManager->getFixStr());
    }

    if (_gpsManager->isDataOld() || !_gpsManager->hasFix())
    {
        _gfx->setTextColor(RED, BG_COLOR);
    }
    else
    {
        _gfx->setTextColor(WHITE, BG_COLOR);
    }
    
    _gfx->println(_gpsManager->getLocationStr());

    if (simple)
    {
        
    } else {
        _gfx->setTextColor(WHITE, BG_COLOR);
        _gfx->println(_gpsManager->getSpeedStr());

        _gfx->println(_gpsManager->getSatellitesStr());
        _gfx->println(_gpsManager->getAntennaStr());
    }
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