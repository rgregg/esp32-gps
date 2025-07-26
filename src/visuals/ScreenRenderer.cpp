#include "ScreenRenderer.h"

ScreenRenderer::ScreenRenderer(Display* display, FS* fileSystem) :
    _display(display), _fileSystem(fileSystem)
{

}

void ScreenRenderer::clearScreen()
{
    _display->fillScreen(BG_COLOR);
}

void ScreenRenderer::drawAboutScreen() 
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    setFontAndSize(TitleFont, 1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Nomaduino GPS");

    _display->setCursor(122, 70);
    _display->setFont(NormalFont);
    _display->println("ESP32 GPS Receiver");
    moveCursorX(122);
    _display->println("Version");
    moveCursorX(122);
    _display->println(AUTO_VERSION);

    drawIcon(8, 45, 92, 101, "/images/nomaduino-92x101.rgb");
}

void ScreenRenderer::drawBootScreen()
{
    int pos_x = 17, pos_y = 22;
    drawIcon(pos_x, pos_y, 122, 122, "/images/nomaduino-122.rgb");

    _display->setCursor(140, 54);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->setFont(TitleFont);
    _display->setTextSize(1);
    _display->println("Nomaduino");

    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->setCursor(140, _display->getCursorY());
    _display->println(AUTO_VERSION);
}

void ScreenRenderer::drawCoreScreen(GPSManager* gps) 
{
    // Draw the date and time
    _display->setFont(Heading2Font);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->setCursor(LEFT_PADDING, TOP_PADDING);

    if (gps->getTimeStr().isEmpty() || gps->getDateStr().isEmpty()) {
        _display->println("No date/time yet");
    } else {
        _display->print(gps->getDateStr());
        _display->print(" ");
        _display->println(gps->getTimeStr());
    }

    // Fill the space with black where we're going to write the GPS data
    int cursorY = _display->getCursorY();
    _display->fillRect(0, cursorY,
                   _display->width() - ICON_BAR_WIDTH - ICON_PADDING, _display->height() - cursorY,
                   BG_COLOR);

    // Draw the lat/long
    if(gps->hasFix())
    {
        DMS latitude = gps->getLatitude();
        _display->setCursor(LEFT_PADDING, cursorY);
        this->drawDMS(latitude);
        _display->println("");

        DMS longitude = gps->getLongitude();
        moveCursorX(LEFT_PADDING);
        this->drawDMS(longitude);
    }
    else
    {
        _display->setCursor(LEFT_PADDING, cursorY);
        _display->setTextColor(YELLOW, BG_COLOR);
        _display->println("Waiting for GPS fix");
        moveCursorX(LEFT_PADDING);
        _display->setTextColor(WHITE, BG_COLOR);
        _display->setFont(NormalFont);
        _display->println("Check GPS receiver antenna");
    }
}

void ScreenRenderer::drawDebugScreen()
{
    auto humanReadableBytes = [](uint16_t bytes) -> String {
        if (bytes < 1024) {
            return String(bytes) + " b";
        }
        if (bytes < 1024 * 1024) {
            return String(bytes / 1024.0, 1) + " kb";
        }
        return String(bytes / 1048576.0, 1) + " mb";
    };


    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->setFont(TitleFont);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Debug");

    _display->setCursor(LEFT_PADDING, 60);
    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->printf("HEAP: %s / %s\n", humanReadableBytes(ESP.getFreeHeap()), humanReadableBytes(ESP.getHeapSize()));
    moveCursorX(LEFT_PADDING);
    _display->printf("PSRAM: %s / %s\n", humanReadableBytes(ESP.getFreePsram()), humanReadableBytes(ESP.getPsramSize()));
}

void ScreenRenderer::drawGPSScreen(GPSManager* gps)
{
    _display->setTextColor(WHITE, BG_COLOR);
    _display->setFont(Heading1Font);
    _display->setTextSize(1);
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->println("GPS Information");

    _display->setFont(NormalFont);
    _display->setTextSize(1);
    
    // Fix
    _display->setTextColor(gps->hasFix() ? GREEN : RED, BG_COLOR);
    moveCursorX(LEFT_PADDING);
    _display->println(gps->getFixStr());
    
    // Satellites
    _display->setTextColor(WHITE, BG_COLOR);
    moveCursorX(LEFT_PADDING);
    _display->println(gps->getSatellitesStr());
    
    // Antenna
    moveCursorX(LEFT_PADDING);
    _display->println(gps->getAntennaStr());
}

void ScreenRenderer::drawNavigationScreen(GPSManager* gps)
{
    _display->setTextColor(WHITE, BG_COLOR);

    _display->setFont(Heading1Font);
    _display->setTextSize(1); 

    bool hasFix = gps->hasFix();
    
    int angle = gps->getDirectionFromTrueNorth();
    if (!hasFix) angle = 0;
    // Draw the compass
    drawCompass(44, 60, 40, angle);

    int speed_x = 190, speed_y = 56;
    int16_t x1, y1;
    uint16_t w, h;
    String speed = String(gps->getSpeed(), 1);
    if (!hasFix) speed = "No Fix";
    _display->getTextBounds(speed, 0, 0, &x1, &y1, &w, &h);
    _display->setCursor(speed_x - (w/2), speed_y);
    _display->println(speed);

    if (hasFix)
    {
        this->setFontAndSize(NormalFont, 1);
        String units = "knots";
        _display->getTextBounds(units, 0, 0, &x1, &y1, &w, &h);
        moveCursorX(speed_x - (w/2));
        _display->print(units);
    }
}

void ScreenRenderer::drawUpdateScreen(String updateType, uint8_t percentComplete)
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->setFont(TitleFont);
    _display->setTextSize(1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Nomaduino");

    _display->setCursor(LEFT_PADDING, 60);
    _display->setFont(NormalFont);
    _display->setTextSize(1);
    _display->print(updateType);
    _display->print(" updating... ");
    _display->print(String(percentComplete));
    _display->print("%");
}

void ScreenRenderer::drawWiFiPortalScreen(String portalSSID)
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    setFontAndSize(TitleFont, 1);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->println("Nomaduino GPS");

    setFontAndSize(NormalFont, 1);
    moveCursorX(LEFT_PADDING);
    _display->setTextColor(YELLOW, BG_COLOR);
    _display->println("Configure via WiFi");

    moveCursorX(LEFT_PADDING);
    _display->setTextColor(WHITE, BG_COLOR);
    _display->print("SSID: ");
    _display->println(portalSSID);

    moveCursorX(LEFT_PADDING);
    _display->print("http://");
    _display->println(WiFi.softAPIP().toString());
}

void ScreenRenderer::drawWiFiScreen(String wifiStatus)
{
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->setTextColor(WHITE, BG_COLOR);
    setFontAndSize(Heading1Font, 1);
    _display->println("WIFI Information");
    
    setFontAndSize(NormalFont, 1);
    moveCursorX(LEFT_PADDING);
    _display->println(WiFi.SSID());
    moveCursorX(LEFT_PADDING);
    _display->println(wifiStatus);
    moveCursorX(LEFT_PADDING);
    _display->println("IP: " + WiFi.localIP().toString());
    moveCursorX(LEFT_PADDING);
    _display->println("RSSI: " + String(WiFi.RSSI()));
}

static String imagePathForWiFiStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        int signal = WiFi.RSSI();
        if (signal <= -80) {
            return "/images/wifi-32-low.rgb";
        } else if (signal <= -67) {
            return "/images/wifi-32-medium.rgb";
        } else {
            return "/images/wifi-32-high.rgb";
        }
    } else {
        return "/images/wifi-32-disconnected.rgb";
    }
}

static String imagePathForBatteryStatus() {
    // TODO: Implement battery support more broadly
    return "/images/battery-32-none.rgb";
}

/// @brief Draws the status icon bar on the screen - either on the right side in landscape or at the bottom in portrait
void ScreenRenderer::drawIconBar(bool landscapeOrientation, GPSManager* gps)
{
    int width = _display->width();
    int height = _display->height();

    const int iconDimension_x = ICON_SIZE;
    const int iconDimension_y = ICON_SIZE;
    const int iconPadding = ICON_PADDING;

    int pos_x, pos_y;
    std::function<void(void)> incrementPosition;
    if (landscapeOrientation)
    {
        pos_x = width - iconPadding - iconDimension_x;
        pos_y = iconPadding;
        incrementPosition = [&pos_y]() { pos_y += iconPadding + iconDimension_y; };
    } else {
        pos_x = iconPadding;
        pos_y = height - iconPadding - iconDimension_y;
        incrementPosition = [&pos_x]() { pos_x += iconPadding + iconDimension_x; };
    }

    // Draw the WiFi icon
    drawIcon(pos_x, pos_y, iconDimension_x, iconDimension_y, imagePathForWiFiStatus());
    incrementPosition();

    // Draw the GPS icon
    String gpsImagePath = gps->hasFix() ? "/images/gps-32-connected.rgb" : "/images/gps-32-disconnected.rgb";
    drawIcon(pos_x, pos_y, iconDimension_x, iconDimension_y, gpsImagePath);
    incrementPosition();

    // Draw the battery icon
    drawIcon(pos_x, pos_y, iconDimension_x, iconDimension_y, imagePathForBatteryStatus());
    incrementPosition();
}

void ScreenRenderer::drawIcon(int x, int y, int width, int height, String filename)
{
    CachedBitmap *cached = nullptr;

    // Check if image is cached
    auto it = _bitmapCache.find(filename);
    if (it != _bitmapCache.end()) {
        cached = &it->second;
    } else {
        // Allocate and load image from FS
        const int bytes_per_pixel = 3;
        uint8_t *imgBuf = (uint8_t*) malloc(width * height * bytes_per_pixel);
        if (!imgBuf) return; // Check malloc success

        File file = _fileSystem->open(filename, "r");
        if (!file || file.read(imgBuf, width * height * bytes_per_pixel) != width * height * bytes_per_pixel) {
            free(imgBuf);
            return; // Read error
        }

        // Cache it
        _bitmapCache[filename] = { imgBuf, width, height };
        cached = &_bitmapCache[filename];
    }

    _display->drawRGBBitmap(x, y, cached->data, cached->width, cached->height);
}

void ScreenRenderer::drawDMS(DMS value) {
    _display->print(String(value.direction));
    _display->print(" ");
    _display->print(String(value.degrees));
    _display->print("\xB0");
    _display->print(String(value.minutes));
    _display->print("\'");
    _display->print(String(value.seconds, 2));
    _display->print("\"");
}

/// @brief Draws a compass rose with the upper left corner at pos_x, pos_y and of dimensions width, with an arrow pointing in direction.
/// @param pos_x 
/// @param pos_y 
/// @param width 
/// @param direction 
void ScreenRenderer::drawCompass(int pos_x, int pos_y, int radius, int headingDegrees)
{
    uint16_t OUTER_COLOR = BLUE;
    uint16_t INNER_COLOR = DARKCYAN;
    uint16_t TICK_MARK_COLOR = LIGHTGREY;
    uint16_t TEXT_COLOR = WHITE;
    uint16_t ARROW_COLOR = RED;

    int centerX = pos_x + radius/2;
    int centerY = pos_y + radius/2;

    _display->drawCircle(centerX, centerY, radius, OUTER_COLOR);
    _display->drawLine(centerX, centerY - radius, centerX, centerY + radius, INNER_COLOR); // N-S
    _display->drawLine(centerX - radius, centerY, centerX + radius, centerY, INNER_COLOR); // W-E

    _display->fillTriangle(
        centerX, centerY - radius - 10,        // tip of the arrow
        centerX - 5, centerY - radius + 5,     // bottom left
        centerX + 5, centerY - radius + 5,     // bottom right
        ARROW_COLOR
    );

    for (int angle = 0; angle < 360; angle += 30) {
        float rad = angle * DEG_TO_RAD;
        int x1 = centerX + cos(rad) * (radius - 5);
        int y1 = centerY + sin(rad) * (radius - 5);
        int x2 = centerX + cos(rad) * radius;
        int y2 = centerY + sin(rad) * radius;
        _display->drawLine(x1, y1, x2, y2, TICK_MARK_COLOR);
    }

    // Print the N for north
    
    _display->setTextSize(1);
    _display->setFont(NormalFont);        // SMALL_TEXT_FONT?

    String north = "N";

    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(north, 0, 0, &x1, &y1, &w, &h);
    _display->setCursor(centerX - (w/2) - 3, centerY - radius - 18);
    _display->print("N");

    // Print the course direction
    String course = String(headingDegrees) + "\xB0";
    setFontAndSize(Heading1Font, 1);
    _display->getTextBounds(course, 0, 0, &x1, &y1, &w, &h);
    _display->setCursor(centerX - (w/2), centerY + radius + h + 10);
    _display->print(course);

    // Rotate by -90 so north is the right direction
    float headingRad = (headingDegrees - 90) * DEG_TO_RAD;
    int needleLength = radius - 10;
    int xTip = centerX + cos(headingRad) * needleLength;
    int yTip = centerY + sin(headingRad) * needleLength;

    _display->drawLine(centerX, centerY, xTip, yTip, ARROW_COLOR);   
}

void ScreenRenderer::moveCursorX(int x)
{
    _display->setCursor(x, _display->getCursorY());
}

void ScreenRenderer::setFontAndSize(DisplayFont font, int size)
{
    _display->setFont(font);
    _display->setTextSize(size);
}

void ScreenRenderer::drawPlaceholderScreen(String text) 
{
    setFontAndSize(Heading1Font, 1);
    _display->setCursor(LEFT_PADDING, TOP_PADDING);
    _display->setTextColor(RED, BG_COLOR);
    _display->setTextSize(1);
    _display->println(text);
}