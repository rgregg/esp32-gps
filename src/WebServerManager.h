#pragma once
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "AppSettings.h"
#include "GPSManager.h"
#include <functional>
#include "ScreenManager.h"
#include <Update.h>


enum OtaUpdateType {
    UPDATE_FLASH = U_FLASH,
    UPDATE_FILESYS = U_SPIFFS
};


class WebServerManager {
public:
    using WiFiConnectCallback = std::function<void()>;
    WebServerManager(AppSettings* settings, GPSManager* gpsManager, ScreenManager* screenMgr);
    void begin();
    void end();
    AsyncWebServer* getServer();
    void setWiFiConnectCallback(WiFiConnectCallback cb);
    void setWiFiScanResult(const String& result);
private:
    AppSettings* settings;
    GPSManager* gpsManager;
    ScreenManager* screenManager;
    
    AsyncWebServer server;
    WiFiConnectCallback wifiConnectCallback;
    String lastWiFiScanResult;
    uint8_t _ota_progress = 0;
    bool previousUploadSuccessful = false;
    void setupRoutes();
    String parseWiFiScanToJson();

    void otaRequestHandler(AsyncWebServerRequest *request);
    void otaContentHandler(OtaUpdateType updateType, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
};
