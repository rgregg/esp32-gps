#pragma once

#include "GPSManager.h"
#include <Arduino_GFX_Library.h>
#include "AppSettings.h"

enum ScreenMode {
  ScreenMode_BOOT = 0,
  ScreenMode_GPS,
  ScreenMode_OTA,
  ScreenMode_PORTAL
};

#define BRIGHTNESS_HIGH 255
#define BRIGHTNESS_OFF  0

class ScreenManager 
{

public:
    ScreenManager(AppSettings* settings);

    void begin();
    void loop();

    void refreshScreen();
    void setScreenMode(ScreenMode mode);
    ScreenMode getScreenMode();
    bool isScreenMode(ScreenMode compareMode);
    void setBacklight(uint8_t brightness);
    void setGPSManager(GPSManager* gpsManager);
    void setOTAStatus(uint8_t percentComplete);
    void setPortalSSID(String ssid);


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

    bool refreshIfTimerElapsed(uint32_t maxTime);
    void updateScreenForGPS();
    const char* currentWifIStatus();
    
};
