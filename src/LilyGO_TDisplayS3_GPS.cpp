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

#include "Credentials.h"
#include "Constants.h"
#include "GPSManager.h"
#include "AppSettings.h"
#include "ScreenManager.h"

HardwareSerial GPSSerial(1);
GPSManager *gpsManager = nullptr;
ScreenManager *screenManager = nullptr;
AppSettings *settings = nullptr;

AsyncWebServer server(80);

String fullHostname;
bool otaEnabled = OTA_ENABLED_DEFAULT;
bool otaReady = false;

TLogPlusStream::TelnetSerialStream telnetSerialStream = TLogPlusStream::TelnetSerialStream();
uint32_t screenRefreshTimer = millis();
uint32_t lastWiFiConnectionTimer = 0;
uint8_t overTheAirUpdateProgress = 0;
uint32_t ota_progress_mills = 0;

bool launchedConfigPortal = false;
bool isWiFiConfigured = true;
bool isTelnetSetup = false;

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
  // Arduino IDE USB_CDC_ON_BOOT = Enable, default Serial input & output data from USB-C
  Serial.begin(115200);
  Serial.println("Booting T-Display-S3 GPS Adapter");

  TLogPlus::Log.begin();

  TLogPlus::Log.debugln("Loading file system");
#ifndef NO_FS
  settings = new AppSettings();
  settings->loadDefaults();
  TLogPlus::Log.warningln("No file system support enabled - settings will not be saved.");
#else
  if (!LittleFS.begin(true))
  {
    TLogPlus::Log.warningln("An Error has occurred while mounting LittleFS. Device will restart.");
    delay(30000);
    ESP.restart();
  }
  settings = new AppSettings(&LittleFS);
#endif

  if (!settings->load())
  {
    TLogPlus::Log.infoln("No settings file found - using defaults");
    settings->loadDefaults();
    settings->save();
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
                              settings->getInt(SETTING_DATA_AGE_THRESHOLD));
  gpsManager->begin();

  screenManager->setGPSManager(gpsManager);

  // When we're all done, switch to the GPS mode
  if (hasWiFiConfigured)
  {
    screenManager->setScreenMode(ScreenMode_GPS);
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
  TLogPlus::Log.loop();
  
  if (isTelnetSetup) telnetSerialStream.loop();

  if (!launchedConfigPortal && shouldAttemptWiFiConnection())
  {
    // Attempts to reconnect to the WiFi network if we get disconnected, after a small delay
    TLogPlus::Log.infoln("Attempting to reconnect to WiFi");
    connectToWiFi();
  }
}

void startConfigPortal()
{
  launchedConfigPortal = true;
  TLogPlus::Log.infoln("Starting WiFi configuration portal...");
  // wifiManager.setTitle("ESP32-S3 GPS");
  // wifiManager.setConfigPortalBlocking(false);
  // wifiManager.setAPCallback([](WiFiManager *manager)
  //                           {
  //   TLogPlus::Log.println("APCallback occured");
  //   TLogPlus::Log.print("Portal SSID: ");
  //   String portalSSID = manager->getConfigPortalSSID();
  //   TLogPlus::Log.println(portalSSID);
  //   screenManager->setPortalSSID(portalSSID); });
  // wifiManager.setBreakAfterConfig(true);
  // wifiManager.setSaveConfigCallback([]()
  //                                   {
  //   // callback when configuration is changed (always happens, even if connection fails)
  //   TLogPlus::Log.println("SaveConfigCallback occured");
  //   String ssid = wifiManager.getWiFiSSID();
  //   String password = wifiManager.getWiFiPass();
  //   TLogPlus::Log.debug("Saving WiFi settings: ");
  //   TLogPlus::Log.debugln(ssid);

  //   settings->set(SETTING_WIFI_SSID, ssid);
  //   settings->set(SETTING_WIFI_PSK, password);
  //   settings->save();
  //   isWiFiConfigured = true;
  //   TLogPlus::Log.debugln("Switching to GPS mode");
  //   screenManager->setScreenMode(ScreenMode_GPS); });

  // screenManager->setScreenMode(ScreenMode_PORTAL);

  // // Start the portal
  // wifiManager.startConfigPortal(fullHostname.c_str());
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
    settings->save();
  }
  else if (cmd == "password")
  {
    TLogPlus::Log.info("Changing WiFi password to ");
    TLogPlus::Log.infoln(value);
    settings->set(SETTING_WIFI_PSK, value);
    settings->save();
  }
  else if (cmd == "settings")
  {
    TLogPlus::Log.infoln("Updating app settings to new JSON.");
    TLogPlus::Log.infoln(value);
    settings->load(value);
    settings->save();
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
  if (screenManager->getScreenMode() == ScreenMode_PORTAL)
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
    TLogPlus::Log.println("Network services enabled");
  }
  else
  {
    // We don't have an IP address or network connection
    otaReady = false;
    isTelnetSetup = false;
    telnetSerialStream.stop();
    server.end();
    TLogPlus::Log.println("Network services disabled");
  }
}

void setupWebServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });

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
  screenManager->setScreenMode(ScreenMode_OTA);
}

void onOTAProgress(size_t current, size_t final)
{
  if (millis() - ota_progress_mills > 1000) {
    ota_progress_mills = millis();
    TLogPlus::Log.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);

    float percentageComplete = current / final;
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
  screenManager->setScreenMode(ScreenMode_GPS);
}
