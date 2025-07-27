#pragma once

#include "ScreenRenderer.h"

class MonoScreenRenderer : public ScreenRenderer 
{
public:
    MonoScreenRenderer(Display* display, FS* fileSys);

    void drawHeader(String text);
    void drawStatus(uint8_t wifiStatus, int gpsStatus, float batteryVoltage);

    void drawAboutScreen();
    void drawGPSScreen(GPSManager* gps);

    void drawCoreScreen(GPSManager* gps);

    void drawDebugScreen();
    void drawWiFiScreen(String wifiStatus);
    void drawWiFiPortalScreen(String portalSSID);

    void drawBootScreen() override;
    void drawNavigationScreen(GPSManager* gps) override;
    void drawUpdateScreen(String updateType, uint8_t percentComplete) override;
    void drawPlaceholderScreen(String text) override;

    void clearScreen() override;
    void drawIcon(int x, int y, int width, int height, String filename) override;
};