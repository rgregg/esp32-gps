#pragma once

#include "GPSManager.h"
#include <Arduino_GFX_Library.h>
#include "AppSettings.h"
#include <LittleFS.h>
#include <map>

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
    ScreenManager(AppSettings* settings);

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
    void setOTAStatus(uint8_t percentComplete);
    void setPortalSSID(String ssid);
    void setRotation(uint8_t rotation, bool redraw = true);
    void moveNextScreen(int8_t direction);

private:
    GPSManager* _gpsManager;
    AppSettings* _settings;
    Arduino_DataBus* _bus;
    Arduino_GFX* _gfx;
    Arduino_GFX* _display;
    ScreenMode _screenMode;
    ScreenOrientation _orientation;
    uint32_t _refreshTimer;
    uint32_t _refreshGPSTime;
    uint32_t _refreshOtherTime;
    uint8_t _otaStatusPercentComplete;
    String _portalSSID;
    ScreenMode _screenLoop[6] = { SCREEN_CORE, SCREEN_NAVIGATION, SCREEN_WIFI, SCREEN_GPS, SCREEN_ABOUT, SCREEN_DEVICE_DEBUG };
    ScreenMode _previousScreenMode;

    struct CachedBitmap {
        uint8_t* data;
        int width;
        int height;
    };

    std::map<String, CachedBitmap> _bitmapCache;

    bool refreshIfTimerElapsed(uint32_t maxTime);
    const char* currentWiFiStatus();
    void drawAboutScreen();
    void drawBootScreen();
    void drawCompass(int pos_x, int pos_y, int radius, int headingDegrees);
    void drawCoreScreen();
    void drawDebugScreen();
    void drawDMS(DMS value);
    void drawGPSScreen();
    void drawIcon(int x, int y, int width, int height, String filename);
    void drawIconBar();
    void drawNavigationScreen();
    void drawUpdateScreen();
    void drawWiFiPortalScreen();
    void drawWiFiScreen();
    void moveCursorX(int x);
    void setFontAndSize(const GFXfont *f, int size);
};
