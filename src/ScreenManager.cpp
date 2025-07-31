#include "ScreenManager.h"
#include <Arduino.h>
#include <TLogPlus.h>
#include <LittleFS.h>
#include "Constants.h"
#include <algorithm>

ScreenManager::ScreenManager(AppSettings *settings, Display* display, ScreenRenderer* renderer) : 
    _settings(settings), _display(display), _orientation(LANDSCAPE), 
    _screenMode(SCREEN_BOOT), _otaUpdateType(""), _renderer(renderer)
{
    _gpsManager = nullptr;
    _magnetometerManager = nullptr;
    _refreshGPSTime = _settings->getInt(SETTING_SCREEN_REFRESH_INTERVAL, SCREEN_REFRESH_INTERVAL_DEFAULT);
    _refreshOtherTime = _settings->getInt(SETTING_REFRESH_INTERVAL_OTHER, REFRESH_INTERVAL_OTHER_DEFAULT);
}

void ScreenManager::begin()
{
    _refreshTimer = millis();

    _display->begin();
    setBacklight(_settings->getInt("backlight", 100));
    
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
    _display->setBacklight(percent);
    _settings->setInt("backlight", percent);
}

void ScreenManager::setGPSManager(GPSManager *manager)
{
    _gpsManager = manager;
}

void ScreenManager::setMagnetometerManager(MagnetometerManager *manager)
{
    _magnetometerManager = manager;
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
    _renderer->clearScreen();

    if (_screenMode != SCREEN_BOOT && _screenMode != SCREEN_ABOUT)
    {
        _renderer->drawIconBar(_orientation == LANDSCAPE, _gpsManager);
    }
    
    switch (_screenMode) {
        case SCREEN_BOOT:
            _renderer->drawBootScreen();
            break;
        case SCREEN_ABOUT:
            _renderer->drawAboutScreen();
            break;
        case SCREEN_CORE:
            _renderer->drawCoreScreen(_gpsManager);
            break;
        case SCREEN_NAVIGATION:
            _renderer->drawNavigationScreen(_gpsManager, _magnetometerManager);
            break;
        case SCREEN_WIFI:
            _renderer->drawWiFiScreen(currentWiFiStatus());
            break;
        case SCREEN_GPS:
            _renderer->drawGPSScreen(_gpsManager);
            break;
        case SCREEN_UPDATE_OTA:
            _renderer->drawUpdateScreen(_otaUpdateType, _otaStatusPercentComplete);
            break;
        case SCREEN_NEEDS_CONFIG:
            _renderer->drawWiFiPortalScreen(_portalSSID);
            break;
        case SCREEN_DEVICE_DEBUG:
            _renderer->drawDebugScreen();
            break;
        case SCREEN_CALIBRATION:
            _renderer->drawCalibrationScreen(_gpsManager, _magnetometerManager);
            break;
        default:
            _renderer->drawPlaceholderScreen("This screen unintentionally left blank");
            break;
    }
    _display->flush();
    //TLogPlus::Log.println("flushed display");
}

void ScreenManager::showDefaultScreen()
{
    setScreenMode(_screenLoop[0]);
}

void ScreenManager::moveScreenInLoop(int8_t direction)
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





