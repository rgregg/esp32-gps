#pragma once

#include "GPSManager.h"
#include "Display.h"
#include <map>
#include <FS.h>

#define BG_COLOR 0
#define WHITE 255
#define RED 254
#define GREEN 253
#define YELLOW 252
#define BLUE 251
#define DARKCYAN 250
#define LIGHTGREY 249
#define BLACK 0

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
    virtual void drawNavigationScreen(GPSManager* gps);
    virtual void drawUpdateScreen(String updateType, uint8_t percentComplete);
    virtual void drawWiFiPortalScreen(String portalSSID);
    virtual void drawWiFiScreen(String wifiStatus);
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