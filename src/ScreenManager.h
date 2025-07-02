#pragma once

#include "GPSManager.h"
#include <Arduino_GFX_Library.h>
#include "AppSettings.h"
#include <LittleFS.h>

enum ScreenMode {
  ScreenMode_BOOT = 0,
  ScreenMode_ADVANCED,
  ScreenMode_GPS,
  ScreenMode_WIFI,
  ScreenMode_OTA,
  ScreenMode_PORTAL,
  ScreenMode_ABOUT,

  ScreenMode_MAX
};

#define BRIGHTNESS_HIGH 255
#define BRIGHTNESS_OFF  0

class ScreenManager 
{

public:
    ScreenManager(AppSettings* settings);

    void begin();
    void loop();

    void refreshScreen(bool fullRefresh = false);
    void setScreenMode(ScreenMode mode);
    ScreenMode getScreenMode();
    bool isScreenMode(ScreenMode compareMode);
    void setBacklight(uint8_t percent);
    void setGPSManager(GPSManager* gpsManager);
    void setOTAStatus(uint8_t percentComplete);
    void setPortalSSID(String ssid);
    void setRotation(uint8_t rotation);
    void moveNextScreen(int8_t direction);

private:
    GPSManager* _gpsManager;
    AppSettings* _settings;
    Arduino_DataBus* _bus;
    Arduino_GFX* _gfx;
    ScreenMode _screenMode;
    uint32_t _refreshTimer;
    uint32_t _refreshGPSTime;
    uint32_t _refreshOtherTime;
    uint8_t _otaStatusPercentComplete;
    String _portalSSID;
    FS* _fileSystem;
    ScreenMode _screenLoop[4] = { ScreenMode_GPS, ScreenMode_ADVANCED, ScreenMode_WIFI, ScreenMode_ABOUT };

    bool refreshIfTimerElapsed(uint32_t maxTime);
    void drawGPSScreen(bool simple = false);
    const char* currentWiFiStatus();
    void drawBootScreen(bool showAbout);
    void drawWiFiScreen();
    
};
