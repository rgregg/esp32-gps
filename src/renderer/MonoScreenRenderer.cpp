#include "MonoScreenRenderer.h"

/*
    Render the various screens on a monochrome OLED display which is 128x64 pixels.
    NOTE: Adafruit GFX library appears to set the cursor position at the baseline of the text - where as the Adruino GFX library is setting it at the top-left corner of the text.
*/

#define OFFSET_Y 20

MonoScreenRenderer::MonoScreenRenderer(Display* display, FS* fileSys) : 
    ScreenRenderer(display, fileSys) {

}

void MonoScreenRenderer::drawHeader(String text)
{
    _display->setTextColor(WHITE, BLACK);
    uint16_t baseline = drawTextAtLocation(0, 0, NormalFont, text);
    _display->drawLine(0, baseline, _display->width(), baseline, WHITE);
}

void MonoScreenRenderer::drawStatus(uint8_t wifiStatus, int gpsStatus, float batteryVoltage)
{
    // _display->setFont(NormalFont);
    // _display->setTextSize(1);
    // _display->setTextColor(WHITE, BLACK);
    // _display->setCursor(0, 54);
    // _display->printf("W:%d G:%d B:%.1f", wifiStatus, gpsStatus, batteryVoltage);
}

void MonoScreenRenderer::drawAboutScreen()
{
    drawHeader("Nomaduino GPS");
    
    uint16_t next_y = OFFSET_Y;
    next_y = drawTextAtLocation(0, OFFSET_Y, NormalFont, "Version");
    drawTextAtLocation(0, next_y, NormalFont, AUTO_VERSION);
}

void MonoScreenRenderer::drawGPSScreen(GPSManager* gps)
{
    drawHeader("GPS");
    
    uint16_t next_y = OFFSET_Y;
    if (gps) {
        next_y = drawTextAtLocation(0, next_y, NormalFont, gps->getFixStr());
        next_y = drawTextAtLocation(0, next_y, NormalFont, gps->getSatellitesStr());
        next_y = drawTextAtLocation(0, next_y, NormalFont, gps->getAntennaStr());
    } else {
        next_y = drawTextAtLocation(0, next_y, NormalFont, "No GPS available");
    }
}

void MonoScreenRenderer::drawCoreScreen(GPSManager* gps)
{
    drawHeader("Location");

    uint16_t next_y = OFFSET_Y;
    if(gps->hasFix())
    {
        DMS latitude = gps->getLatitude();
        next_y = drawTextAtLocation(0, next_y, NormalFont, 
            latitude.direction + String(latitude.degrees) + " " + String(latitude.minutes) + "'" + String(latitude.seconds) + "\"");

        DMS longitude = gps->getLongitude();
        next_y = drawTextAtLocation(0, next_y, NormalFont, 
            longitude.direction + String(longitude.degrees) + " " + String(longitude.minutes) + "'" + String(longitude.seconds) + "\"");

        next_y = drawTextAtLocation(0, next_y, NormalFont, 
            gps->getSpeedStr());
    }
    else
    {
        next_y = drawTextAtLocation(0, next_y, NormalFont, "Waiting for GPS fix");
    }
}

void MonoScreenRenderer::drawDebugScreen()
{
    drawHeader("Debug");

    uint16_t next_y = OFFSET_Y;
    next_y = drawTextAtLocation(0, next_y, NormalFont, 
        "HEAP: " + String(ESP.getFreeHeap()) + " / " + String(ESP.getHeapSize()));
    next_y = drawTextAtLocation(0, next_y, NormalFont, 
        "PSRAM: " + String(ESP.getFreePsram()) + " / " + String(ESP.getPsramSize()));
}

void MonoScreenRenderer::drawWiFiScreen(String wifiStatus)
{
    drawHeader("WiFi");

    uint16_t next_y = OFFSET_Y;
    String ssid = WiFi.SSID();
    if (ssid && ssid.length() > 0) {
        next_y = drawTextAtLocation(0, next_y, NormalFont, wifiStatus + " - " + ssid);
    } else {
        next_y = drawTextAtLocation(0, next_y, NormalFont, wifiStatus);
    }
    next_y = drawTextAtLocation(0, next_y, NormalFont, WiFi.localIP().toString());
}

void MonoScreenRenderer::drawWiFiPortalScreen(String portalSSID)
{
    drawHeader("Config Required");
    uint16_t next_y = OFFSET_Y;
    next_y = drawTextAtLocation(0, next_y, NormalFont, "Connect to the portal");
    next_y = drawTextAtLocation(0, next_y, NormalFont, "SSID: " + portalSSID);
    next_y = drawTextAtLocation(0, next_y, NormalFont, "IP: " + WiFi.softAPIP().toString());
}

void MonoScreenRenderer::drawCalibrationScreen(GPSManager* gps, MagnetometerManager* mag)
{
    drawHeader("Calibration");
    uint16_t next_y = OFFSET_Y;

    if (!mag || !gps) {
        next_y = drawTextAtLocation(0, next_y, NormalFont, "Not available - No MAG");
        return;
    }

    if (gps && gps->hasFix()) {
        next_y = drawTextAtLocation(0, next_y, NormalFont, "GPS Course: " + String(gps->getDirectionFromTrueNorth()) + "\xB0");
    } else {
        next_y = drawTextAtLocation(0, next_y, NormalFont, "GPS Course: No Fix");
    }
    
    next_y = drawTextAtLocation(0, next_y, NormalFont, "Mag Heading: " + String(mag->getHeading()) + "\xB0");

    
    float minX, maxX, minY, maxY, minZ, maxZ;
    mag->getMinMaxX(minX, maxX);
    mag->getMinMaxY(minY, maxY);
    mag->getMinMaxZ(minZ, maxZ);

    next_y = drawTextAtLocation(0, next_y, NormalFont, "X: " + String(minX, 1) + " - " + String(maxX, 1));
    next_y = drawTextAtLocation(0, next_y, NormalFont, "Y: " + String(minY, 1) + " - " + String(maxY, 1));
    next_y = drawTextAtLocation(0, next_y, NormalFont, "Z: " + String(minZ, 1) + " - " + String(maxZ, 1));
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

void MonoScreenRenderer::drawNavigationScreen(GPSManager* gps, MagnetometerManager* mag)
{
    drawHeader("Navigation");
    
    uint16_t next_y = OFFSET_Y;
    if(gps->hasFix())
    {
        next_y = drawTextAtLocation(0, next_y, NormalFont, "Speed: " + String(gps->getSpeed(), 1) + " knots");
        next_y = drawTextAtLocation(0, next_y, NormalFont, "Course: " + String(mag->getHeading()) + " deg");
    }
    else
    {
        next_y = drawTextAtLocation(0, next_y, NormalFont, "Waiting for GPS fix");
    }
}

void MonoScreenRenderer::drawUpdateScreen(String updateType, uint8_t percentComplete)
{
    drawHeader("Updating");
    uint16_t next_y = OFFSET_Y;
    next_y = drawTextAtLocation(0, next_y, NormalFont, updateType + "...");
    next_y = drawTextAtLocation(0, next_y, NormalFont, String(percentComplete) + "%");
}

void MonoScreenRenderer::drawPlaceholderScreen(String text)
{
    drawHeader("Placeholder");
    uint16_t next_y = OFFSET_Y;
    next_y = drawTextAtLocation(0, next_y, NormalFont, text);
}
