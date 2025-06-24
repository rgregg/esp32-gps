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
#include <TLog.h> 
#include <TelnetSerialStream.h>
#include <SPIFFS.h>
#include "Credentials.h"
#include "Constants.h"
#include "GPSManager.h"
#include "AppSettings.h"

TelnetSerialStream telnetSerialStream = TelnetSerialStream();
Arduino_DataBus *bus = new Arduino_ESP32PAR8Q(
    7 /* DC */, 6 /* CS */, 8 /* WR */, 9 /* RD */,
    39 /* D0 */, 40 /* D1 */, 41 /* D2 */, 42 /* D3 */, 45 /* D4 */, 46 /* D5 */, 47 /* D6 */, 48 /* D7 */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */, 0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);

HardwareSerial GPSSerial(1);
GPSManager gpsManager(&GPSSerial, 18, 21, 57600, false);
AppSettings *settings;

uint32_t screenRefreshTimer = millis();
uint8_t overTheAirUpdateProgress = 0;

// int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1;
// uint8_t tsa, tsb, tsc, ds;

ScreenMode currentScreenMode = ScreenMode_BOOT;
char* fullHostname = nullptr;


void loadDefaultSettings();
void initDisplay();
void connectToWiFi();
void updateScreen();
void updateScreenForGPS();
void setupOTA();
void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
String CurrentWiFiStatus();
void OTA_OnStart();
void OTA_OnError(int code, const char* message);

void setup() 
{
    // Arduino IDE USB_CDC_ON_BOOT = Enable, default Serial input & output data from USB-C
    Serial.begin(115200);
    Serial.println("Booting T-Display-S3 GPS Adapter");

    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS. Device will restart.");  
        delay(30000);
        ESP.restart();
    }

    settings = new AppSettings(&SPIFFS);
    if (!settings->load())
    {
      Log.println("No settings file found - using defaults");
      loadDefaultSettings();
    }

    connectToWiFi();
    initDisplay();
    
    gpsManager.begin();
    currentScreenMode = ScreenMode_GPS;
}

void loop() 
{
  if (OTA_ENABLED)
  {
    // Check for OTA updates
    ArduinoOTA.poll();
  }

  if (currentScreenMode != ScreenMode_OTA)
  {
    gpsManager.loop(false);
    Log.loop();

    // approximately every 2 seconds or so, print out the current stats
    if (millis() - screenRefreshTimer > SCREEN_REFRESH_INTERVAL) {
      screenRefreshTimer = millis(); // reset the timer
      updateScreen();
    }
  }
}

void loadDefaultSettings()
{
  settings->setBool("gps_echo", true);
  settings->setBool("gps_log_to_serial", true);
  settings->setBool("ota_enabled", true);
  settings->set("ota_password", "password");
  settings->setInt("average_speed_count", 10);
  settings->setInt("data_age_threshold", 5000);
  settings->setInt("gps_baud_rate", 57600);
  settings->save();
}

void initDisplay()
{
  GFX_EXTRA_PRE_INIT();

#ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
#endif

    gfx->begin();
    gfx->setRotation(1);
    updateScreen();
}

void connectToWiFi() 
{
    // Configure the hostname
    const char* nameprefix = WIFI_HOSTNAME;
    uint16_t maxlen = strlen(nameprefix) + 7;
    fullHostname = new char[maxlen];
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(fullHostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
    WiFi.setHostname(fullHostname);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    WiFi.onEvent(WiFi_Connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFi_GotIPAddress, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFi_Disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Log.println("\nConnecting to WiFi Network..");
}

void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  Log.println("Connected to the WiFi network");

  if (OTA_ENABLED)
  {
    setupOTA();
  }
  Log.addPrintStream(std::make_shared<TelnetSerialStream>(telnetSerialStream));
}

void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  Log.print("Local ESP32 IP: ");
  Log.println(WiFi.localIP());
}

void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  Log.println("Disconnected from WiFi network");
  // Attempt to reconnect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

String CurrentWiFiStatus()
{
  int status = WiFi.status();
  switch(status) {
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID_AVAIL";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
  }
  return "N/A";
}

String timeStr, dateStr, fixStr, locationStr, speedStr, angleStr, altitudeStr, satellitesStr, antennaStr;
bool gpsHasFix;


void updateScreen()
{
  gfx->startWrite();
  gfx->fillScreen(BLACK);

  switch(currentScreenMode)
  {
    case ScreenMode_BOOT:
      gfx->setCursor(0, 0);
      gfx->setTextColor(WHITE);
      gfx->setTextSize(3);
      gfx->print("BOOTING...");
      break;
    case ScreenMode_GPS:
      updateScreenForGPS();
      break;
    case ScreenMode_OTA:
      gfx->setCursor(0, 0);
      gfx->setTextColor(YELLOW);
      gfx->setTextSize(2);
      gfx->print("UPDATING... ");
      gfx->print(overTheAirUpdateProgress);
      gfx->print("%");
      break;
    case ScreenMode_PORTAL:
      break;
  }
  gfx->endWrite();
}

void updateScreenForGPS()
{
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(0, 0);
  gfx->print(gpsManager.getDateStr());
  gfx->print(" ");
  gfx->println(gpsManager.getTimeStr());

  if (gpsManager.hasFix()) {
    gfx->setTextColor(GREEN);
  } else {
    gfx->setTextColor(RED);
  }

  gfx->println(gpsManager.getFixStr());

  if (gpsManager.isDataOld()) {
    gfx->setTextColor(RED);
  } else {
    gfx->setTextColor(WHITE);
  }
  gfx->println(gpsManager.getLocationStr());

  gfx->setTextColor(WHITE);
  gfx->println(gpsManager.getSpeedStr());
  
  gfx->print(gpsManager.getSatellitesStr());
  gfx->print(" ");
  gfx->println(gpsManager.getAntennaStr());

  gfx->println("WiFi: " + CurrentWiFiStatus());
  gfx->println("IP: " + WiFi.localIP().toString());
}

char inputBuffer[MAX_COMMAND_LEN];
uint8_t inputPos = 0;

// void readCommandFromSerial()
// {
//   Adafruit_GPS &GPS = gpsManager.getGPS();
//   while (Serial.available() > 0) {
//     char local = Serial.read();
//     if (local == '\n') {
//       // Null-terminate and send the buffered command
//       inputBuffer[inputPos] = '\0';
//       //Serial.println(inputBuffer); // Echo back
//       String cmd = String(inputBuffer, inputPos);
//       GPS.sendCommand(cmd.c_str());
//       // Reset buffer position
//       inputPos = 0;
//     } else if (inputPos < MAX_COMMAND_LEN - 1) {
//       // Store the character if there's room
//       inputBuffer[inputPos++] = local;
//     } else {
//       // Buffer overflow protection
//       inputPos = 0;
//     }
//   }
// }

void setupOTA()
{
  ArduinoOTA.onStart(OTA_OnStart);
  ArduinoOTA.onError(OTA_OnError);
  ArduinoOTA.begin(WiFi.localIP(), WIFI_HOSTNAME, OTA_PASSWORD, InternalStorage);
}

void OTA_OnStart()
{
  Log.println("OTA: Start");
  currentScreenMode = ScreenMode_OTA;
}

void OTA_OnError(int code, const char* message)
{
  Log.printf("Error[%u]: %s", code, message);
  currentScreenMode = ScreenMode_GPS;
} 

