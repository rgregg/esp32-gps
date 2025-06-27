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
#include "Credentials.h"
#include "Constants.h"
#include "GPSManager.h"
#include "AppSettings.h"
#include "ScreenManager.h"

HardwareSerial GPSSerial(1);
GPSManager *gpsManager = nullptr;
ScreenManager *screenManager = nullptr;
AppSettings *settings = nullptr;
char* fullHostname = nullptr;
bool enableOTA = OTA_ENABLED_DEFAULT;

TLogPlusStream::TelnetSerialStream telnetSerialStream = TLogPlusStream::TelnetSerialStream();
uint32_t screenRefreshTimer = millis();
uint8_t overTheAirUpdateProgress = 0;

void connectToWiFi();
void OTA_OnError(int code, const char* message);
void OTA_OnStart();
void processDebugCommand(String debugCmd);
void setupOTA();
void setupTelnetStream();
void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);

void setup() 
{
    // Arduino IDE USB_CDC_ON_BOOT = Enable, default Serial input & output data from USB-C
    Serial.begin(115200);
    Serial.println("Booting T-Display-S3 GPS Adapter");

    setupTelnetStream();

    TLogPlus::Log.begin();
    TLogPlus::Log.addPrintStream(std::make_shared<TLogPlusStream::TelnetSerialStream>(telnetSerialStream));

    if (!SPIFFS.begin(true)) {
        TLogPlus::Log.warningln("An Error has occurred while mounting SPIFFS. Device will restart.");  
        delay(30000);
        ESP.restart();
    }

    settings = new AppSettings(&SPIFFS);
    if (!settings->load())
    {
      TLogPlus::Log.infoln("No settings file found - using defaults");
      settings->loadDefaults();
    }

    screenManager = new ScreenManager(settings);
    screenManager->begin();

    enableOTA = settings->getBool(SETTING_OTA_ENABLED);

    connectToWiFi();

    gpsManager = new GPSManager(&GPSSerial, GPS_RX_PIN, GPS_TX_PIN, 
      settings->getInt(SETTING_BAUD_RATE),
      settings->getBool(SETTING_GPS_LOG_ENABLED),
      settings->getInt(SETTING_DATA_AGE_THRESHOLD));
    gpsManager->begin();

    screenManager->setGPSManager(gpsManager);

    // When we're all done, switch to the GPS mode
    screenManager->setScreenMode(ScreenMode_GPS);
}

void loop() 
{
  if (enableOTA)
  {
    ArduinoOTA.poll();
  }

  gpsManager->loop();
  screenManager->loop();
  TLogPlus::Log.loop();
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

void connectToWiFi() 
{
    // Configure the hostname
    const char* nameprefix = settings->get(SETTING_WIFI_HOSTNAME, WIFI_HOSTNAME_DEFAULT);
    uint16_t maxlen = strlen(nameprefix) + 7;
    fullHostname = new char[maxlen];
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(fullHostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
    WiFi.setHostname(fullHostname);
    TLogPlus::Log.debugln("WiFi Hostname: %s", fullHostname);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    WiFi.onEvent(WiFi_Connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFi_GotIPAddress, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFi_Disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    
    TLogPlus::Log.debugln("Connecting to WiFi Network");

    const char* ssid = settings->get(SETTING_WIFI_SSID);
    const char* password = settings->get(SETTING_WIFI_PSK);
    WiFi.begin(ssid, password);
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
  TLogPlus::Log.infoln("IP: %p", WiFi.localIP());
}

void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.infoln("Disconnected from WiFi network");
  // Attempt to reconnect
  const char* ssid = settings->get(SETTING_WIFI_SSID);
  const char* password = settings->get(SETTING_WIFI_PSK);
  WiFi.begin(ssid, password);
}

void setupOTA()
{
  const char* hostname = settings->get(SETTING_WIFI_HOSTNAME);
  const char* otaPassword = settings->get(SETTING_OTA_PASSWORD);

  ArduinoOTA.onStart(OTA_OnStart);
  ArduinoOTA.onError(OTA_OnError);
  ArduinoOTA.begin(WiFi.localIP(), hostname, otaPassword, InternalStorage);
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

