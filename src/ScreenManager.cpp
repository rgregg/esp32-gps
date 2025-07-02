#include "ScreenManager.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TLogPlus.h>
#include <LittleFS.h>
#include "Constants.h"

#define BACKLIGHT_PWM_CHANNEL 0
#define BACKLIGHT_PWM_FREQ 5000
#define BACKLIGHT_PWM_RESOLUTION 8

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
    _gfx->setRotation(1);

    refreshScreen();
}

void ScreenManager::loop()
{
    switch (_screenMode)
    {
    case ScreenMode_BOOT:
    case ScreenMode_OTA:
    case ScreenMode_PORTAL:
        // Refresh every 5 seconds
        refreshIfTimerElapsed(_refreshOtherTime);
        break;
    case ScreenMode_GPS:
        // Refresh every _interval_ seconds
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
        refreshScreen();
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

void ScreenManager::refreshScreen()
{
    _gfx->startWrite();
    _gfx->fillScreen(BLACK);
    switch (_screenMode) {
        case ScreenMode_BOOT:
            drawBootScreen();
            break;
        case ScreenMode_GPS:
            updateScreenForGPS();
            break;
        case ScreenMode_OTA:
            _gfx->setCursor(0, 0);
            _gfx->setTextColor(YELLOW);
            _gfx->setTextSize(3);
            _gfx->println("ESP32-GPS");
            _gfx->print("Updating... ");
            _gfx->print(_otaStatusPercentComplete);
            _gfx->print("%");
            break;
        case ScreenMode_PORTAL:
            _gfx->setCursor(0, 0);
            _gfx->setTextColor(RED);
            _gfx->setTextSize(3);
            _gfx->println("ESP32-GPS Needs Configuration");
            _gfx->setTextColor(WHITE);
            _gfx->print("SSID: ");
            _gfx->println(_portalSSID);
            _gfx->print("http://");
            _gfx->println(WiFi.softAPIP().toString());
            break;
    }
    _gfx->endWrite();
}

void ScreenManager::drawBootScreen()
{
    _gfx->setCursor(5, 5);
    _gfx->setTextColor(WHITE);
    _gfx->setTextSize(3);
    _gfx->print("ESP32 GPS");
    _gfx->setTextSize(2);
    _gfx->setCursor(5, 30);
    _gfx->printf("Version: %s", AUTO_VERSION);

    int IMG_WIDTH = 64;
    int IMG_HEIGHT = IMG_WIDTH;

    int screenWidth = _gfx->width();
    int screenHeight = _gfx->height();
    int bmp_x = (screenWidth - IMG_WIDTH) / 2;
    int bmp_y = (screenHeight - IMG_HEIGHT) / 2;

    uint8_t *imgBuf = (uint8_t*) malloc(IMG_WIDTH * IMG_HEIGHT * 3);
    File file = LittleFS.open("/images/gps.rgb", "r");
    file.read(imgBuf, IMG_WIDTH * IMG_HEIGHT * 3);
    _gfx->draw24bitRGBBitmap(bmp_x, bmp_y, imgBuf, IMG_WIDTH, IMG_HEIGHT);
    free(imgBuf);
}

void ScreenManager::updateScreenForGPS()
{
    _gfx->setTextColor(WHITE);
    _gfx->setTextSize(2);
    _gfx->setCursor(0, 0);
    _gfx->print(_gpsManager->getDateStr());
    _gfx->print(" ");
    _gfx->println(_gpsManager->getTimeStr());

    if (_gpsManager->hasFix())
    {
        _gfx->setTextColor(GREEN);
        _gfx->println(_gpsManager->getFixStr());
    }
    else
    {
        _gfx->setTextColor(RED);
        _gfx->println("No Fix");
    }

    if (_gpsManager->isDataOld())
    {
        _gfx->setTextColor(RED);
    }
    else
    {
        _gfx->setTextColor(WHITE);
    }
    
    _gfx->println(_gpsManager->getLocationStr());

    _gfx->setTextColor(WHITE);
    _gfx->println(_gpsManager->getSpeedStr());

    _gfx->print(_gpsManager->getSatellitesStr());
    _gfx->print(" ");
    _gfx->println(_gpsManager->getAntennaStr());

    _gfx->print("WiFi: ");
    _gfx->println(currentWifIStatus());
    _gfx->println("IP: " + WiFi.localIP().toString());
}

const char* ScreenManager::currentWifIStatus()
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