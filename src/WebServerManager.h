#pragma once
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <LittleFS.h>
#include "AppSettings.h"
#include "GPSManager.h"
#include <functional>
#include "ScreenManager.h"

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
    void setupRoutes();
    void setupOTA();
    String parseWiFiScanToJson();

    void onOTAStart();
    void onOTAProgress(size_t current, size_t final);
    void onOTAEnd(bool success);
    uint32_t ota_progress_mills = 0;
};
