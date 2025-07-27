#pragma once

#include "GPSManager.h"
#include "AppSettings.h"
#include "Display.h"
#include "MagnetometerManager.h"
#include <LittleFS.h>
#include <map>
#include "renderer/ScreenRenderer.h"

enum ScreenMode {
  SCREEN_NONE = -1,
  SCREEN_BOOT = 0,
  SCREEN_CORE,
  SCREEN_NAVIGATION,
  SCREEN_WIFI,
  SCREEN_GPS,
  SCREEN_ABOUT,
  SCREEN_UPDATE_OTA,
  SCREEN_NEEDS_CONFIG,
  SCREEN_DEVICE_DEBUG,
  SCREEN_CALIBRATION,

  SCREEN_MAX
};

enum ScreenOrientation {
  LANDSCAPE = 0,
  PORTRAIT
};

#define BRIGHTNESS_HIGH 255
#define BRIGHTNESS_OFF  0

class ScreenManager 
{
public:
    ScreenManager(AppSettings* settings, Display* display, ScreenRenderer* renderer);

    void begin();
    void loop();

    void refreshScreen(bool fullRefresh = false);
    void setScreenMode(ScreenMode mode);
    void showDefaultScreen();
    ScreenMode getScreenMode();
    bool isScreenMode(ScreenMode compareMode);
    void setScreenOrientation(ScreenOrientation orientation);
    void setBacklight(uint8_t percent);
    void setGPSManager(GPSManager* gpsManager);
    void setMagnetometerManager(MagnetometerManager* magnetometerManager);
    void setOTAStatus(String updateType, uint8_t percentComplete);
    void setPortalSSID(String ssid);
    void setRotation(uint8_t rotation, bool redraw = true);
    void moveScreenInLoop(int8_t direction);

private:
    GPSManager* _gpsManager;
    MagnetometerManager* _magnetometerManager;
    AppSettings* _settings;
    Display* _display;
    ScreenRenderer* _renderer;
    ScreenMode _screenMode;
    ScreenOrientation _orientation;
    uint32_t _refreshTimer;
    uint32_t _refreshGPSTime;
    uint32_t _refreshOtherTime;
    uint8_t _otaStatusPercentComplete;
    String _otaUpdateType;
    String _portalSSID;
    ScreenMode _screenLoop[7] = { SCREEN_CORE, SCREEN_NAVIGATION, SCREEN_WIFI, SCREEN_GPS, SCREEN_ABOUT, SCREEN_DEVICE_DEBUG, SCREEN_CALIBRATION };

    bool refreshIfTimerElapsed(uint32_t maxTime);
    const char* currentWiFiStatus();
};
