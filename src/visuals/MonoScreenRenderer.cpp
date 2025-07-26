#include "MonoScreenRenderer.h"

// Render the various screens on a monochrome OLED display which is 128x64 pixels.

MonoScreenRenderer::MonoScreenRenderer(Display* display, FS* fileSys) : 
    ScreenRenderer(display, fileSys) {

}

void MonoScreenRenderer::drawHeader(String text)
{
    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BLACK);
    _display->setCursor(0, 0);
    _display->println(text);
    _display->drawLine(0, 12, _display->width(), 12, WHITE);
}

void MonoScreenRenderer::drawStatus(uint8_t wifiStatus, int gpsStatus, float batteryVoltage)
{
    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BLACK);
    _display->setCursor(0, 54);
    _display->printf("W:%d G:%d B:%.1f", wifiStatus, gpsStatus, batteryVoltage);
}

void MonoScreenRenderer::drawAboutScreen()
{
    drawHeader("About");
    _display->setCursor(0, 20);
    _display->println("Nomaduino GPS");
    _display->println("Version: " AUTO_VERSION);
}

void MonoScreenRenderer::drawGPSScreen(GPSManager* gps)
{
    drawHeader("GPS");
    _display->setCursor(0, 20);
    _display->println("Fix: " + gps->getFixStr());
    _display->println("Sats: " + gps->getSatellitesStr());
    _display->println("Ant: " + gps->getAntennaStr());
}

void MonoScreenRenderer::drawCoreScreen(GPSManager* gps)
{
    drawHeader("Core");
    _display->setCursor(0, 20);
    if(gps->hasFix())
    {
        DMS latitude = gps->getLatitude();
        _display->println(latitude.direction + String(latitude.degrees) + " " + String(latitude.minutes) + "'" + String(latitude.seconds) + "\"");
        DMS longitude = gps->getLongitude();
        _display->println(longitude.direction + String(longitude.degrees) + " " + String(longitude.minutes) + "'" + String(longitude.seconds) + "\"");
    }
    else
    {
        _display->println("Waiting for GPS fix");
    }
}

void MonoScreenRenderer::drawDebugScreen()
{
    drawHeader("Debug");
    _display->setCursor(0, 20);
    _display->printf("HEAP: %d / %d\n", ESP.getFreeHeap(), ESP.getHeapSize());
    _display->printf("PSRAM: %d / %d\n", ESP.getFreePsram(), ESP.getPsramSize());
}

void MonoScreenRenderer::drawWiFiScreen(String wifiStatus)
{
    drawHeader("WiFi");
    _display->setCursor(0, 20);
    _display->println(WiFi.SSID());
    _display->println(wifiStatus);
    _display->println(WiFi.localIP().toString());
}

void MonoScreenRenderer::drawWiFiPortalScreen(String portalSSID)
{
    drawHeader("Config Portal");
    _display->setCursor(0, 20);
    _display->println("SSID: " + portalSSID);
    _display->println("IP: " + WiFi.softAPIP().toString());
}

void MonoScreenRenderer::clearScreen()
{
    _display->fillScreen(BLACK);
}

void MonoScreenRenderer::drawIcon(int x, int y, int width, int height, String filename)
{
    // No-op
}

void MonoScreenRenderer::drawBootScreen()
{
    drawHeader("Booting...");
}

void MonoScreenRenderer::drawNavigationScreen(GPSManager* gps)
{
    drawHeader("Navigation");
    _display->setCursor(0, 20);
    if(gps->hasFix())
    {
        _display->println("Speed: " + String(gps->getSpeed(), 1) + " knots");
        _display->println("Course: " + String(gps->getDirectionFromTrueNorth()) + " deg");
    }
    else
    {
        _display->println("Waiting for GPS fix");
    }
}

void MonoScreenRenderer::drawUpdateScreen(String updateType, uint8_t percentComplete)
{
    drawHeader("Updating");
    _display->setCursor(0, 20);
    _display->println(updateType + "...");
    _display->println(String(percentComplete) + "%");
}

void MonoScreenRenderer::drawPlaceholderScreen(String text)
{
    drawHeader("Placeholder");
    _display->setCursor(0, 20);
    _display->println(text);
}
