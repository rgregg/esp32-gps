/*
  ESP32 GPS Receiver
  Copyright (c) 2025 Ryan Gregg
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <TLogPlus.h>
#include <TelnetSerialStream.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>

#include "Credentials.h"
#include "Constants.h"
#include "GPSManager.h"
#include "AppSettings.h"
#include "ScreenManager.h"
#include "UDPManager.h"
#include "ButtonManager.h"

HardwareSerial GPSSerial(1);
GPSManager *gpsManager = nullptr;
ScreenManager *screenManager = nullptr;
AppSettings *settings = nullptr;
UDPManager *udpManager = nullptr;
ButtonManager *btnRight = nullptr;
ButtonManager *btnLeft = nullptr;

AsyncWebServer server(80);

String fullHostname;

// Button callback functions
void onButtonRightPress(ButtonPressType type) {
  TLogPlus::Log.printf("Right button press: %u\n", type);
  if (type == SHORT_PRESS)
    screenManager->moveNextScreen(1);
}

void onButtonLeftPress(ButtonPressType type) {
  TLogPlus::Log.printf("Left button press: %u\n", type);
  if (type == SHORT_PRESS)
    screenManager->moveNextScreen(-1);
}

TLogPlusStream::TelnetSerialStream telnetSerialStream = TLogPlusStream::TelnetSerialStream();
uint32_t screenRefreshTimer = millis();
uint32_t lastWiFiConnectionTimer = 0;
uint8_t overTheAirUpdateProgress = 0;
uint32_t ota_progress_mills = 0;
RTC_DATA_ATTR int bootCount = 0;
int runtimeDurationMillis = millis();

bool launchedConfigPortal = false;
bool isWiFiConfigured = true;
bool isTelnetSetup = false;
uint8_t loopCounter = 0;
String lastWiFiScanResult;

bool connectToWiFi(bool firstAttempt = false);
void completeConfigurationPortal();
void startConfigPortal();
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);
void processDebugCommand(String debugCmd);
void processSerialInput();
void setupTelnetStream();
bool shouldAttemptWiFiConnection();
void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void setupWebServer();
void configureNetworkDependents(bool connected);

void setup()
{
  bootCount++;
  // Arduino IDE USB_CDC_ON_BOOT = Enable, default Serial input & output data from USB-C
  Serial.begin(115200);
  Serial.println("Booting T-Display-S3 GPS Adapter");

  if (bootCount > 5) 
  {
    // Looks like we're stuck in a boot loop
    Serial.println("Detected boot loop, triggering rollback...");
    esp_err_t result = esp_ota_mark_app_invalid_rollback_and_reboot();
    if (result == ESP_FAIL) {
      Serial.println("rollback was not attempted");
    } else if (result == ESP_ERR_OTA_ROLLBACK_FAILED) {
      Serial.println("rollback failed");
    }
  }
  
  TLogPlus::Log.begin();
  TLogPlus::Log.printf("Firmware version: %s\r\n", AUTO_VERSION);

  TLogPlus::Log.debugln("Loading app settings");
  settings = new AppSettings();
  if (!settings->load())
  {
    TLogPlus::Log.infoln("Error loading settings - using defaults");
    settings->loadDefaults();
  }
  
  TLogPlus::Log.debugln("Loading file system");
  if (!LittleFS.begin(true))
  {
    TLogPlus::Log.warningln("An Error has occurred while mounting LittleFS. Device will restart.");
    delay(30000);
    ESP.restart();
  }

  TLogPlus::Log.debugln("Loading screen manager");
  screenManager = new ScreenManager(settings);
  screenManager->begin();
  
  TLogPlus::Log.debugln("Connecting to WiFi");
  bool hasWiFiConfigured = connectToWiFi(true);

  TLogPlus::Log.debugln("Connecting to GPS device");
  gpsManager = new GPSManager(&GPSSerial, GPS_RX_PIN, GPS_TX_PIN,
                              settings->getInt(SETTING_BAUD_RATE),
                              settings->getBool(SETTING_GPS_LOG_ENABLED),
                              settings->getInt(SETTING_DATA_AGE_THRESHOLD),
                              (GPSDataMode)settings->getInt(SETTING_GPS_DATA_MODE),
                              (GPSRate)settings->getInt(SETTING_GPS_FIX_RATE),
                              (GPSRate)settings->getInt(SETTING_GPS_UPDATE_RATE));
  gpsManager->begin();

  TLogPlus::Log.debugln("Setting up UDP manager");
  if (settings->getBool(SETTING_UDP_ENABLED))
  {
    String host = settings->get(SETTING_UDP_HOST);
    uint16_t port = settings->getInt(SETTING_UDP_PORT);
    TLogPlus::Log.printf("Enabling UDP GPS sentence delivery to %s:%u", host, port);

    udpManager = new UDPManager(host.c_str(), port);
    gpsManager->setUDPManager(udpManager);
  }

  screenManager->setGPSManager(gpsManager);

  // Setup button managers
  btnRight = new ButtonManager(BTN_RIGHT_PIN, onButtonRightPress);
  btnLeft = new ButtonManager(BTN_LEFT_PIN, onButtonLeftPress);

  // When we're all done, switch to the GPS mode
  if (hasWiFiConfigured)
  {
    delay(2000);
    screenManager->showDefaultScreen();
  }
  else
  {
    startConfigPortal();
  }

  TLogPlus::Log.debugln("Initialization complete");
}

void loop()
{
  ElegantOTA.loop();

  processSerialInput();

  gpsManager->loop();
  screenManager->loop();
  btnRight->loop();
  btnLeft->loop();
  TLogPlus::Log.loop();
  
  if (isTelnetSetup) telnetSerialStream.loop();

  // Confirm that we're a stagble upgrade if we aren't stuck rebooting.
  if (runtimeDurationMillis > 0 && millis() - runtimeDurationMillis > 60000)
  {
    // Device has been running for 60 seconds, confirm this firmware is stable
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
    {
      runtimeDurationMillis = 0;
    }
  }
}

void startConfigPortal()
{
  launchedConfigPortal = true;
  TLogPlus::Log.infoln("Starting WiFi configuration portal...");

  WiFi.mode(WIFI_AP_STA);

  IPAddress apIP(192,168,4,1);        // Default AP IP is 192.168.4.1
  IPAddress gateway(192,168,4,1);
  IPAddress subnet(255,255,255,0);
  WiFi.softAPConfig(apIP, gateway, subnet);
  WiFi.softAP(fullHostname);

  configureNetworkDependents(true);

  screenManager->setPortalSSID(fullHostname);
  screenManager->setScreenMode(SCREEN_NEEDS_CONFIG);
}

String parseWiFiScanToJson() {
  int scanResult = WiFi.scanComplete();
  JsonDocument doc;
  JsonArray networks = doc["networks"].to<JsonArray>();
  if (scanResult == WIFI_SCAN_RUNNING) {
    doc["status"] = "running";
  } else if (scanResult == WIFI_SCAN_FAILED) {
    // This usually means no scan has been started, so return the cached
    // result if available
    if (lastWiFiScanResult != "") {
      return lastWiFiScanResult;
    }
    doc["status"] = "failed";
  } else if (scanResult >= 0) {
    TLogPlus::Log.printf("Scan complete! Found %d networks.\n", scanResult);
    doc["status"] = "complete";
    for (int i = 0; i < scanResult; ++i) {
      
      TLogPlus::Log.printf("%2d: %s (%d dBm)%s\n", i + 1,
                          WiFi.SSID(i).c_str(),
                          WiFi.RSSI(i),
                          (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [open]" : "");
      JsonObject net = networks.add<JsonObject>();
      net["ssid"] = WiFi.SSID(i);
      net["rssi"] = WiFi.RSSI(i);
      net["bssid"] = WiFi.BSSIDstr(i);
      net["channel"] = WiFi.channel(i);
      net["encryption"] = WiFi.encryptionType(i);
    }
    // Clean up after scan to free memory
    WiFi.scanDelete();
  }

  String jsonStr;
  serializeJson(doc, jsonStr);

  if (scanResult > 0) {
    // Cache the result for next time
    lastWiFiScanResult = jsonStr;
  }

  return jsonStr;
}

void completeConfigurationPortal()
{
  if (!launchedConfigPortal)
    return;
  launchedConfigPortal = false;

  TLogPlus::Log.debugln("Shutting down config portal");
  // wifiManager.stopConfigPortal();

  TLogPlus::Log.debugln("Connecting to wifi");
  connectToWiFi();
}

void setupTelnetStream()
{
  if (!ENABLE_TELNET)
  {
    TLogPlus::Log.debugln("Telnet logging is disabled.");
    return;
  }
  
  TLogPlus::Log.debugln("Initializing telnet server.");

  telnetSerialStream.setLineMode();
  telnetSerialStream.setLogActions();

  telnetSerialStream.onInputReceived([](String str)
                                     {
                                       processDebugCommand(str); 
                                     });

  telnetSerialStream.onConnect([](IPAddress ipAddress)
                               {
                                 TLogPlus::Log.info("onConnection: Connection from ");
                                 TLogPlus::Log.infoln(ipAddress.toString());
                               });

  telnetSerialStream.onDisconnect([](IPAddress ipAddress)
                                  {
                                    TLogPlus::Log.info("onDisconnect: Disconnection from ");
                                    TLogPlus::Log.infoln(ipAddress.toString());
                                  });
  telnetSerialStream.begin();
  // add telnetSerialStream to log
  TLogPlus::Log.addPrintStream(std::make_shared<TLogPlusStream::TelnetSerialStream>(telnetSerialStream));
  isTelnetSetup = true;
}

void processDebugCommand(String debugCmd)
{
  TLogPlus::Log.debug("Received debug command: ");
  TLogPlus::Log.debugln(debugCmd);

  // Receives a debug command from serial or telnet connection and performs the desired action
  int sepIndex = debugCmd.indexOf(':');
  String cmd;
  String value = "";
  if (sepIndex == -1)
  {
    cmd = debugCmd;
  }
  else
  {
    cmd = debugCmd.substring(0, sepIndex);
    value = debugCmd.substring(sepIndex + 1);
  }

  cmd.toLowerCase();

  if (cmd == "gpscmd")
  {
    gpsManager->sendCommand(value.c_str());
  }
  else if (cmd == "gpsbaud")
  {
    gpsManager->changeBaud(value.toInt());
  }
  else if (cmd == "gpsdata")
  {
    gpsManager->setDataMode((GPSDataMode)value.toInt());
  }
  else if (cmd == "gpsfix")
  {
    gpsManager->setFixRate((GPSRate)value.toInt());
  }
  else if (cmd == "gpsrate")
  {
    gpsManager->setRefreshRate((GPSRate)value.toInt());
  }
  else if (cmd == "refresh")
  {
    screenManager->refreshScreen();
  }
  else if (cmd == "backlight")
  {
    screenManager->setBacklight((uint8_t)value.toInt());
  }
  else if (cmd == "screenmode")
  {
    screenManager->setScreenMode((ScreenMode)value.toInt());
  }
  else if (cmd == "ssid")
  {
    TLogPlus::Log.info("Changing SSID to ");
    TLogPlus::Log.infoln(value);
    settings->set(SETTING_WIFI_SSID, value);
  }
  else if (cmd == "password")
  {
    TLogPlus::Log.info("Changing WiFi password to ");
    TLogPlus::Log.infoln(value);
    settings->set(SETTING_WIFI_PSK, value);
  }
  else if (cmd == "settings")
  {
    TLogPlus::Log.infoln("Updating app settings to new JSON.");
    TLogPlus::Log.infoln(value);
    settings->load(value);
  }
  else if (cmd == "restart")
  {
    TLogPlus::Log.infoln("Restarting device...");
    ESP.restart();
  }
  else if (cmd == "printgps")
  {
    TLogPlus::Log.infoln("Printing GPS data to console.");
    gpsManager->printToLog();
  }
  else if (cmd == "printsettings")
  {
    TLogPlus::Log.infoln("Printing app settings to console.");
    settings->printToLog();
  }
  else if (cmd == "printwifi")
  {
    TLogPlus::Log.infoln("Printing WiFi information");
    TLogPlus::Log.printf("Status: %u\n", WiFi.status());
    TLogPlus::Log.printf("IP: %s\n", WiFi.localIP().toString());
    TLogPlus::Log.printf("Base Station ID: %s\n", WiFi.BSSIDstr().c_str());
    TLogPlus::Log.printf("SSID: %s\n", WiFi.SSID().c_str());
    TLogPlus::Log.printf("RSSI: %i\n", WiFi.RSSI());
  }
  else if (cmd == "reconnect")
  {
    connectToWiFi();
  }
  else if (cmd == "udphost")
  {
    udpManager->setDestHost(value.c_str());
    settings->set(SETTING_UDP_HOST, value);
  }
  else if (cmd == "udpport")
  {
    udpManager->setDestPort(value.toInt());
    settings->setInt(SETTING_UDP_PORT, value.toInt());
  }
  else
  {
    TLogPlus::Log.warningln("Unrecognized debug command: ");
    TLogPlus::Log.warningln(cmd);
  }
}

bool connectToWiFi(bool firstAttempt)
{
  if (!isWiFiConfigured)
  {
    return false;
  }

  TLogPlus::Log.debugln("Connecting to WiFi");
  lastWiFiConnectionTimer = millis();

  // Configure the hostname
  if (firstAttempt)
  {
    String nameprefix = settings->get(SETTING_WIFI_HOSTNAME, WIFI_HOSTNAME_DEFAULT);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char mac_cstr[7];
    snprintf(mac_cstr, sizeof(mac_cstr), "%02x%02x%02x", mac[3], mac[4], mac[5]);
    fullHostname = nameprefix + "-" + mac_cstr;
    WiFi.setHostname(fullHostname.c_str());
    TLogPlus::Log.debug("Device hostname: ");
    TLogPlus::Log.debugln(fullHostname);

    WiFi.onEvent(WiFi_Connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFi_GotIPAddress, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFi_Disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.setAutoReconnect(true);
  }

  uint32_t freeSpace = ESP.getFreeHeap();
  WiFi.mode(WIFI_STA);

  String ssid = settings->get(SETTING_WIFI_SSID);
  String password = settings->get(SETTING_WIFI_PSK);

  if (ssid.isEmpty())
  {
    TLogPlus::Log.warningln("No WiFi SSID configured.");
    isWiFiConfigured = false;
    return false;
  }
  else
  {
    TLogPlus::Log.info("Attempting to connect to WiFi network: ");
    TLogPlus::Log.infoln(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    return true;
  }
}

bool shouldAttemptWiFiConnection()
{
  if (WiFi.status() != WL_DISCONNECTED)
    return false;
  if (screenManager->getScreenMode() == SCREEN_NEEDS_CONFIG)
    return false;

  return ((millis() - lastWiFiConnectionTimer) > WIFI_RECONNECT_TIMEOUT);
}

void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.debugln("Connected to WiFi");
}

void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.printf("Got IP: %s\n", WiFi.localIP().toString());
  configureNetworkDependents(true);
}

void configureNetworkDependents(bool connected)
{
  if (connected)
  {
    // We're on the network with an IP address!
    setupTelnetStream();
    setupWebServer();
    if (udpManager != nullptr)
      udpManager->begin();
    TLogPlus::Log.println("Network services enabled");
  }
  else
  {
    // We don't have an IP address or network connection
    isTelnetSetup = false;
    telnetSerialStream.stop();
    server.end();
    if (udpManager != nullptr)
      udpManager->stop();
    TLogPlus::Log.println("Network services disabled");
  }
}

void setupWebServer()
{
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Create a JSON object from current settings
    String settingsJson = settings->getRawJson();
    request->send(200, "application/json", settingsJson);
  });

  AsyncCallbackWebHandler *handler = new AsyncCallbackWebHandler();
  handler->setUri("/api/settings");
  handler->setMethod(HTTP_POST);
  handler->onRequest([](AsyncWebServerRequest *request) {
    // This is needed to satisfy the library, even if empty
  });
  handler->onBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String jsonBody = String((char *)data, len);
    TLogPlus::Log.infoln("Received settings JSON: " + jsonBody);
    if (settings->load(jsonBody)) {
      request->send(200, "application/json", R"({"success":true})");
    } else {
      request->send(400, "application/json", R"({"success":false, "message":"Invalid JSON"})");
    }
  });
  server.addHandler(handler);


  server.on("/api/wifi_scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    String results = parseWiFiScanToJson();
    request->send(200, "application/json", results);
  });

  server.on("/api/wifi_scan", HTTP_POST, [](AsyncWebServerRequest *request) {
    lastWiFiScanResult = "";
    WiFi.scanNetworks(true);
    request->send(202, "text/plain", "Scan started");
  });

  server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["ssid"] = settings->get(SETTING_WIFI_SSID);
    doc["password"] = settings->get(SETTING_WIFI_PSK);
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });

  server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String jsonBody = String((char*)data, len);
      TLogPlus::Log.infoln("Received WiFi JSON: " + jsonBody);

      JsonDocument doc;  // Adjust size as needed
      DeserializationError error = deserializeJson(doc, jsonBody);

      if (!error) {
        settings->set(SETTING_WIFI_SSID, doc["ssid"].as<String>());
        settings->set(SETTING_WIFI_PSK, doc["password"].as<String>());
        request->send(200, "application/json", R"({"success":true})");
        connectToWiFi();
      } else {
        request->send(400, "application/json", R"({"success":false, "message":"Invalid JSON"})");
      }
    }
  );

  server.on("/api/gpsdata", HTTP_GET, [](AsyncWebServerRequest *request) {    
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
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });

  server.on("/api/version", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", AUTO_VERSION);
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Placeholder for reboot confirmation page
    request->send(200, "text/plain", "Rebooting... Please wait.");
    ESP.restart();
  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"success\":true, \"message\":\"File upload initiated.\"}");
  });
  
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      // Start of upload
      String filePath = request->arg("path");
      if (filePath.isEmpty() || filePath == "/") {
        filePath = "/" + filename;
      }
      TLogPlus::Log.infoln("Starting upload to: " + filePath);
      request->_tempFile = LittleFS.open(filePath, "w");
      if (!request->_tempFile) {
        TLogPlus::Log.errorln("Failed to open file for writing: " + filePath);
        request->send(500, "application/json", R"({"success":false, "message":"Failed to open file for writing"})");
        return;
      }
    }
    if (len) {
      // Writing data to file
      if (request->_tempFile) {
        request->_tempFile.write(data, len);
      }
    }
    if (final) {
      // End of upload
      if (request->_tempFile) {
        request->_tempFile.close();
        TLogPlus::Log.infoln("File upload complete.");
        request->send(200, "application/json", R"({"success":true, "path":""})" + request->arg("path") + R"("}");
      } else {
        request->send(500, "application/json", R"({"success":false, "message":"File handle not found"})");
      }
    }
  });  
  server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");

  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  TLogPlus::Log.println("HTTP server started.");
}

void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.infoln("Disconnected from WiFi network");
  configureNetworkDependents(false);
}

void processSerialInput()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0)
    {
      processDebugCommand(input);
    }
  }
}

void onOTAStart()
{
  TLogPlus::Log.infoln("OTA: Update stareted");
  screenManager->setOTAStatus(0);
  screenManager->setScreenMode(SCREEN_UPDATE_OTA);
}

void onOTAProgress(size_t current, size_t final)
{
  if (millis() - ota_progress_mills > 1000) {
    ota_progress_mills = millis();
    TLogPlus::Log.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);

    float percentageComplete = (float)current / (float)final;
    int status = (int)(percentageComplete * 100.0);
    screenManager->setOTAStatus(status);
  }
}

void onOTAEnd(bool success) 
{
  if (success) {
    TLogPlus::Log.println("OTA update finished succesfully!");
    screenManager->setOTAStatus(100);
  } else {
    TLogPlus::Log.println("There was an error during OTA update!");
    screenManager->setOTAStatus(-1);
  }
}

void onOTAError(int code, const char *message)
{
  TLogPlus::Log.infoln("Error[%u]: %s", code, message);
  screenManager->showDefaultScreen();
}
