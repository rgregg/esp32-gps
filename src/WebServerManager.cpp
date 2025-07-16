#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <ESP32-targz.h>
#include <Update.h>

WebServerManager::WebServerManager(AppSettings* settings, GPSManager* gpsManager, ScreenManager* screenMgr)
    : settings(settings), gpsManager(gpsManager), server(80), screenManager(screenMgr) {

}

void WebServerManager::begin() {
    setupRoutes();
    server.begin();
}

void WebServerManager::end() {
    server.end();
}

AsyncWebServer* WebServerManager::getServer() {
    return &server;
}

void WebServerManager::setWiFiConnectCallback(WiFiConnectCallback cb) {
    wifiConnectCallback = cb;
}

void WebServerManager::setWiFiScanResult(const String& result) {
    lastWiFiScanResult = result;
}

String WebServerManager::parseWiFiScanToJson() {
    int scanResult = WiFi.scanComplete();
    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();
    if (scanResult == WIFI_SCAN_RUNNING) {
        doc["status"] = "running";
    } else if (scanResult == WIFI_SCAN_FAILED) {
        if (!lastWiFiScanResult.isEmpty()) {
            return lastWiFiScanResult;
        }
        doc["status"] = "failed";
    } else if (scanResult >= 0) {
        doc["status"] = "complete";
        for (int i = 0; i < scanResult; ++i) {
            JsonObject net = networks.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["bssid"] = WiFi.BSSIDstr(i);
            net["channel"] = WiFi.channel(i);
            net["encryption"] = WiFi.encryptionType(i);
        }
        WiFi.scanDelete();
    }
    String jsonStr;
    serializeJson(doc, jsonStr);
    if (scanResult > 0) {
        lastWiFiScanResult = jsonStr;
    }
    return jsonStr;
}

void WebServerManager::setupRoutes() {
    server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String settingsJson = settings->getRawJson();
        request->send(200, "application/json", settingsJson);
    });

    AsyncCallbackWebHandler *handler = new AsyncCallbackWebHandler();
    handler->setUri("/api/settings");
    handler->setMethod(HTTP_POST);
    handler->onRequest([](AsyncWebServerRequest *request) {});
    handler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        String jsonBody = String((char *)data, len);
        if (settings->load(jsonBody)) {
            request->send(200, "application/json", R"({"success":true})");
        } else {
            request->send(400, "application/json", R"({"success":false, "message":"Invalid JSON"})");
        }
    });
    server.addHandler(handler);

    server.on("/api/wifi_scan", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String results = parseWiFiScanToJson();
        request->send(200, "application/json", results);
    });
    server.on("/api/wifi_scan", HTTP_POST, [this](AsyncWebServerRequest *request) {
        lastWiFiScanResult = "";
        WiFi.scanNetworks(true);
        request->send(202, "text/plain", "Scan started");
    });

    server.on("/api/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["ssid"] = settings->get(SETTING_WIFI_SSID);
        doc["password"] = settings->get(SETTING_WIFI_PSK);
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/api/wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String jsonBody = String((char*)data, len);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonBody);
            if (!error) {
                settings->set(SETTING_WIFI_SSID, doc["ssid"].as<String>());
                settings->set(SETTING_WIFI_PSK, doc["password"].as<String>());
                request->send(200, "application/json", R"({"success":true})");
                if (wifiConnectCallback) wifiConnectCallback();
            } else {
                request->send(400, "application/json", R"({"success":false, "message":"Invalid JSON"})");
            }
        }
    );

    server.on("/api/gpsdata", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["time"] = gpsManager->getTimeStr();
        doc["date"] = gpsManager->getDateStr();
        doc["fix"] = gpsManager->getFixStr();
        doc["location"] = gpsManager->getLocationStr();
        doc["speed"] = gpsManager->getSpeedStr();
        doc["angle"] = gpsManager->getAngleStr();
        doc["altitude"] = gpsManager->getAltitudeStr();
        doc["satellites"] = gpsManager->getSatellitesStr();
        doc["antenna"] = gpsManager->getAntennaStr();
        doc["lastSerialData"] = gpsManager->secondsSinceLastSerialData();
        doc["lastValidData"] = gpsManager->secondsSinceLastValidData();
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/api/version", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", AUTO_VERSION);
    });

    server.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Rebooting... Please wait.");
        ESP.restart();
    });

    server.on("/upload", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (!request->_tempFile) {
                request->send(400, "application/json", "{\"success\":false, \"message\":\"Nothing uploaded.\"}");
            } else {
                request->send(200, "application/json", "({\"success\":true, \"message\":\"Upload complete (maybe).\"}");
            }
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            bool extract = request->arg("extract") == "ON";
            if (!index) {
                String filePath = request->arg("path");
                if (extract) {
                    filePath = "/extract/" + filename;
                } else if (filePath.isEmpty() || filePath == "/") {
                    filePath = "/" + filename;
                }
                request->_tempFile = LittleFS.open(filePath, "w");
                if (!request->_tempFile) {
                    request->send(500, "application/json", R"({"success":false, "message":"Failed to open file for writing"})");
                    return;
                }
            }
            if (len && request->_tempFile) {
                request->_tempFile.write(data, len);
            }
            if (final && request->_tempFile) {
                if (extract) {
                    File file = request->_tempFile;
                    file.seek(0);
                    TarUnpacker tar;
                    tar.haltOnError(true);
                    tar.setTarVerify(true);
                    bool result = tar.tarStreamExpander(&file, file.size(), LittleFS, "/");
                    if (result) {
                        request->send(200, "application/json", "({\"success\":true, \"message\":\"File contents extracted successfully.\"}");
                    } else {
                        request->send(500, "application/json", R"({"success":false, "message":"Extraction error."})");
                    }
                }
                request->_tempFile.close();
                request->send(200, "application/json", "({\"success\":true, \"path\":\"" + request->arg("path") + "\"}");
            } else if (final) {
                request->send(500, "application/json", R"({"success":false, "message":"File handle not found."})");
            }
        }
    );

    server.on("/update/firmware", HTTP_POST, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        response->addHeader("Connection", "close");
        request->send(response);
        ESP.restart();
    }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
            screenManager->setScreenMode(SCREEN_UPDATE_OTA);
            screenManager->setOTAStatus(0);
            _ota_progress = 0;
            if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)){
                Update.printError(Serial);
            }
        }
        if(len){
            size_t written = Update.write(data, len);
            if (written > 0) {
                _ota_progress = (index + len) * 100 / request->contentLength();
                screenManager->setOTAStatus(_ota_progress);
            }
        }
        if(final){
            if(Update.end(true)){
                screenManager->setOTAStatus(100);
            } else {
                Update.printError(Serial);
                screenManager->setOTAStatus(-1);
            }
            _ota_progress = 100;
        }
    });

    server.on("/update/data", HTTP_POST, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        response->addHeader("Connection", "close");
        request->send(response);
        ESP.restart();
    }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
            screenManager->setScreenMode(SCREEN_UPDATE_OTA);
            screenManager->setOTAStatus(0);
            _ota_progress = 0;
            if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)){
                Update.printError(Serial);
            }
        }
        if(len){
            Update.write(data, len);
            _ota_progress = (index + len) * 100 / request->contentLength();
            screenManager->setOTAStatus(_ota_progress);
        }
        if(final){
            if(Update.end(true)){
                screenManager->setOTAStatus(100);
            } else {
                Update.printError(Serial);
                screenManager->setOTAStatus(-1);
            }
            _ota_progress = 100;
        }
    });

    // Serve all the content from the web folder on the file system
    server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");

    TLogPlus::Log.println("Web server routes configured");
}