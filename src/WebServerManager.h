#pragma once
#include <ESPAsyncWebServer.h>
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
    String parseWiFiScanToJson();
};
