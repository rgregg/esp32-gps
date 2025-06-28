/*
  ESP32 GPS Receiver
  Copyright (c) 2025 Ryan Gregg
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <TLogPlus.h>
#include <TelnetSerialStream.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include "Credentials.h"
#include "Constants.h"
#include "GPSManager.h"
#include "AppSettings.h"
#include "ScreenManager.h"

HardwareSerial GPSSerial(1);
GPSManager *gpsManager = nullptr;
ScreenManager *screenManager = nullptr;
AppSettings *settings = nullptr;
WiFiManager wifiManager;

String fullHostname;
bool enableOTA = OTA_ENABLED_DEFAULT;

TLogPlusStream::TelnetSerialStream telnetSerialStream = TLogPlusStream::TelnetSerialStream();
uint32_t screenRefreshTimer = millis();
uint32_t lastWiFiConnectionTimer = 0;
uint8_t overTheAirUpdateProgress = 0;
bool launchedConfigPortal = false;
bool isWiFiConfigured = true;

bool connectToWiFi(bool firstAttempt = false);
void completeConfigurationPortal();
void startConfigPortal();
void OTA_OnError(int code, const char* message);
void OTA_OnStart();
void processDebugCommand(String debugCmd);
void setupOTA();
void setupTelnetStream();
bool shouldAttemptWiFiConnection();
void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);

void setup() 
{
    // Arduino IDE USB_CDC_ON_BOOT = Enable, default Serial input & output data from USB-C
    Serial.begin(115200);
    Serial.println("Booting T-Display-S3 GPS Adapter");

    TLogPlus::Log.begin();
    if (ENABLE_TELNET) {
      TLogPlus::Log.debugln("Initializing telnet for logging");
      setupTelnetStream();
      TLogPlus::Log.addPrintStream(std::make_shared<TLogPlusStream::TelnetSerialStream>(telnetSerialStream));
    } else {
      TLogPlus::Log.debugln("telnet logging is disabled");
    }

    TLogPlus::Log.debugln("Loading file system");
#ifdef NO_FS
    settings = new AppSettings();
#else
    if (!SPIFFS.begin(true)) {
        TLogPlus::Log.warningln("An Error has occurred while mounting SPIFFS. Device will restart.");  
        delay(30000);
        ESP.restart();
    }
    settings = new AppSettings(&SPIFFS);    
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

    enableOTA = settings->getBool(SETTING_OTA_ENABLED);

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
    } else {
      startConfigPortal();
    }
    
    TLogPlus::Log.debugln("Initialization complete");
}

void loop() 
{
  delay(500);
  if (enableOTA)
  {
    ArduinoOTA.poll();
  }

  if (screenManager->getScreenMode() == ScreenMode_PORTAL)
    wifiManager.process();
  else if (launchedConfigPortal)
    completeConfigurationPortal();

  gpsManager->loop();
  screenManager->loop();
  TLogPlus::Log.loop();

  if (!launchedConfigPortal && shouldAttemptWiFiConnection())
  {
    // Attempts to reconnect to the WiFi network if we get disconnected, after a small delay
    connectToWiFi();
  }
}

void startConfigPortal()
{
  launchedConfigPortal = true;
  TLogPlus::Log.infoln("Starting WiFi configuration portal...");
  wifiManager.setTitle(fullHostname.c_str());
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setAPCallback([](WiFiManager *manager) {
    TLogPlus::Log.println("APCallback occured");
    TLogPlus::Log.print("Portal SSID: ");
    String portalSSID = manager->getConfigPortalSSID();
    TLogPlus::Log.println(portalSSID);
    screenManager->setPortalSSID(portalSSID);
  });
  wifiManager.setBreakAfterConfig(true);
  wifiManager.setSaveConfigCallback([]() {
    // callback when configuration is changed (always happens, even if connection fails)
    TLogPlus::Log.println("SaveConfigCallback occured");
    String ssid = wifiManager.getWiFiSSID();
    String password = wifiManager.getWiFiPass();
    TLogPlus::Log.debug("Saving WiFi settings: ");
    TLogPlus::Log.debugln(ssid);

    settings->set(SETTING_WIFI_SSID, ssid);
    settings->set(SETTING_WIFI_PSK, password);
    settings->save();
    isWiFiConfigured = true;
    TLogPlus::Log.debugln("Switching to GPS mode");
    screenManager->setScreenMode(ScreenMode_GPS);
  });

  screenManager->setScreenMode(ScreenMode_PORTAL);

  // Start the portal
  wifiManager.startConfigPortal(fullHostname.c_str());
}

void completeConfigurationPortal()
{
  if (!launchedConfigPortal) return;
  launchedConfigPortal = false;

  TLogPlus::Log.debugln("Shutting down config portal");
  // wifiManager.stopConfigPortal();
  
  TLogPlus::Log.debugln("Connecting to wifi");
  connectToWiFi();
}

void setupTelnetStream() {
  telnetSerialStream.setLineMode();
  telnetSerialStream.setLogActions();
  telnetSerialStream.onInputReceived([](String str) {
        processDebugCommand(str);
    });

    telnetSerialStream.onConnect([](IPAddress ipAddress){
        // TLogPlus::Log.info("onConnection: Connection from ");
        // TLogPlus::Log.infoln(ipAddress.toString());
    });
    
    telnetSerialStream.onDisconnect([](IPAddress ipAddress){
        // TLogPlus::Log.info("onDisconnect: Disconnection from ");
        // TLogPlus::Log.infoln(ipAddress.toString());
    });
}

void processDebugCommand(String debugCmd)
{
  // Receives a debug command from serial or telnet connection and performs the desired action
  int sepIndex = debugCmd.indexOf(':');
  if (sepIndex == -1)
  {
    TLogPlus::Log.warning("Unrecognized debug statement received. Input ignored: ");
    TLogPlus::Log.warningln(debugCmd);
  }

  String cmd = debugCmd.substring(0, sepIndex);
  String value = debugCmd.substring(sepIndex + 1);
  cmd.toLowerCase();
  
  if(cmd == "gps")
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
}

bool connectToWiFi(bool firstAttempt) 
{
    if (!isWiFiConfigured) { return false; }

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
    TLogPlus::Log.printf("Free heap before WiFi: %u bytes\n", freeSpace);
    TLogPlus::Log.debugln("Setting WiFI mode to STA");
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
  
  return ( (millis() - lastWiFiConnectionTimer) > WIFI_RECONNECT_TIMEOUT);
}

void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.debugln("Connected to the WiFi network");
  if (enableOTA)
  {
    TLogPlus::Log.info("Enabling OTA updates");
    setupOTA();
  }
}

void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.printf("IP: %s", WiFi.localIP().toString());
}

void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.infoln("Disconnected from WiFi network");
}

void setupOTA()
{
  String hostname = settings->get(SETTING_WIFI_HOSTNAME);
  String otaPassword = settings->get(SETTING_OTA_PASSWORD);

  ArduinoOTA.onStart(OTA_OnStart);
  ArduinoOTA.onError(OTA_OnError);
  ArduinoOTA.begin(WiFi.localIP(), hostname.c_str(), otaPassword.c_str(), InternalStorage);
}

void OTA_OnStart()
{
  TLogPlus::Log.infoln("OTA: Start");
  screenManager->setOTAStatus(0);
  screenManager->setScreenMode(ScreenMode_OTA);
}

void OTA_OnError(int code, const char* message)
{
  TLogPlus::Log.infoln("Error[%u]: %s", code, message);
  screenManager->setScreenMode(ScreenMode_GPS);
} 

