#pragma once

#include "GPSManager.h"
#include "MagnetometerManager.h"
#include "Display.h"
#include <map>
#include <FS.h>

#ifdef MONO_DISPLAY
#define BG_COLOR 0
#define WHITE 1
#define RED 1
#define GREEN 1
#define YELLOW 1
#define BLUE 1
#define DARKCYAN 1
#define LIGHTGREY 1
#define BLACK 0
#else
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))
#define WHITE RGB565(255, 255, 255)
#define RED RGB565(255, 0, 0)
#define GREEN RGB565(0, 255, 0)
#define YELLOW RGB565(255, 255, 0)
#define BLUE RGB565(0, 0, 255)
#define DARKCYAN RGB565(0, 125, 123)
#define LIGHTGREY RGB565(198, 195, 198)
#define BLACK RGB565(0, 0, 0)
#define BG_COLOR BLACK
#endif

#define ICON_BAR_WIDTH 32
#define LEFT_PADDING 10
#define TOP_PADDING 30
#define ICON_PADDING 10
#define ICON_SIZE 32

class ScreenRenderer {
public:
    ScreenRenderer(Display* display, FS* fileSystem);
    virtual ~ScreenRenderer() = default;

    virtual void clearScreen();
    virtual void drawBootScreen();
    virtual void drawAboutScreen();
    virtual void drawCoreScreen(GPSManager* gps);
    virtual void drawDebugScreen();
    virtual void drawGPSScreen(GPSManager* gps);
    virtual void drawNavigationScreen(GPSManager* gps, MagnetometerManager* mag);
    virtual void drawUpdateScreen(String updateType, uint8_t percentComplete);
    virtual void drawWiFiPortalScreen(String portalSSID);
    virtual void drawWiFiScreen(String wifiStatus);
    virtual void drawCalibrationScreen(GPSManager* gps, MagnetometerManager* mag);
    virtual void drawIconBar(bool landscapeOrientation, GPSManager* gps);
    virtual void drawPlaceholderScreen(String text);

protected:
    void drawDMS(DMS value);
    virtual void drawIcon(int x, int y, int width, int height, String filename);
    void drawCompass(int pos_x, int pos_y, int radius, int headingDegrees);
    
    void moveCursorX(int x);
    void setFontAndSize(DisplayFont font, int size);
    Display* _display;
    FS *_fileSystem;

    struct CachedBitmap {
        uint8_t* data;
        int width;
        int height;
    };

    std::map<String, CachedBitmap> _bitmapCache;
};