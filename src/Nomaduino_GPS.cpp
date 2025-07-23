/*
  Nomaduino GPS Receiver
  Copyright (c) 2025 Ryan Gregg
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "Display.h"
#include "ST7789Display.h"
#include "SH1107Display.h"
#include <SPI.h>
#include <TLogPlus.h>
#include <TelnetSerialStream.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <ESP32-targz.h>
#include <DNSServer.h>
#include <unordered_map>

#include "Constants.h"
#include "GPSManager.h"
#include "AppSettings.h"
#include "ScreenManager.h"
#include "UDPManager.h"
#include "ButtonManager.h"
#include "BufferedLogStream.h"
#include "WebServerManager.h"

using DebugCmd = std::function<void(String)>;

std::unordered_map<std::string, DebugCmd> debugCommands;
HardwareSerial GPSSerial(1);
GPSManager *gpsManager = nullptr;
ScreenManager *screenManager = nullptr;
AppSettings *settings = nullptr;
UDPManager *udpManager = nullptr;
ButtonManager *btnRight = nullptr;
ButtonManager *btnLeft = nullptr;
std::shared_ptr<BufferedLogStream> bufferedLogs;
WebServerManager* webServerManager = nullptr;
DNSServer dnsServer;
String fullHostname;

TLogPlusStream::TelnetSerialStream telnetSerialStream = TLogPlusStream::TelnetSerialStream();
uint32_t screenRefreshTimer = millis();
uint32_t lastWiFiConnectionTimer = 0;
uint32_t wifiFailureStartTime = 0;  // Track when WiFi failures started
uint32_t lastPortalScanTimer = 0;   // Track when we last scanned for configured network in portal mode
uint8_t overTheAirUpdateProgress = 0;
RTC_DATA_ATTR int bootCount = 0;
int runtimeDurationMillis = millis();

bool launchedConfigPortal = false;
bool portalLaunchedManually = false;  // Track if portal was launched manually vs automatically
bool isWiFiConfigured = true;
bool hasTriedWiFiConnection = false;  // Track if we've attempted WiFi connection
bool networkServicesInitalized = false;
bool isTelnetSetup = false;
uint8_t loopCounter = 0;

bool connectToWiFi(bool firstAttempt = false);
bool shouldAttemptWiFiConnection();
void completeConfigurationPortal();
void configureNetworkDependents(bool connected);
void initDebugCommands();
void onButtonLeftPress(ButtonPressType type);
void onButtonRightPress(ButtonPressType type);
void processDebugCommand(String debugCmd);
void processSerialInput();
void setupTelnetStream();
void startConfigPortal();
void WiFi_Connected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);

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

  bufferedLogs = std::make_shared<BufferedLogStream>(50);
  
  TLogPlus::Log.begin();
  TLogPlus::Log.addPrintStream(bufferedLogs);
  TLogPlus::Log.printf("Firmware version: %s\r\n", AUTO_VERSION);

  initDebugCommands();

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
  Display* display;
#ifdef USE_SH1107_DISPLAY
  display = new SH1107Display();
#else
  display = new ST7789Display();
#endif
  screenManager = new ScreenManager(settings, display);
  screenManager->begin();
  
  TLogPlus::Log.debugln("Connecting to WiFi");
  bool hasWiFiConfigured = connectToWiFi(true);

  // Initialize WiFi failure tracking
  if (hasWiFiConfigured) {
    wifiFailureStartTime = millis();  // Start tracking from setup if we have WiFi configured
  }

  TLogPlus::Log.debugln("Connecting to GPS device");
  gpsManager = new GPSManager(&GPSSerial, GPS_RX_PIN, GPS_TX_PIN, settings);
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

  TLogPlus::Log.printf("PSRAM Free: %u, Total: %u", ESP.getFreePsram(), ESP.getPsramSize());
  TLogPlus::Log.debugln("Initialization complete");
}

void loop()
{
  processSerialInput();
  dnsServer.processNextRequest();
  gpsManager->loop();
  screenManager->loop();
  btnRight->loop();
  btnLeft->loop();
  TLogPlus::Log.loop();
  
  if (isTelnetSetup) telnetSerialStream.loop();

  // Check for WiFi connection and automatically launch portal if needed
  if (isWiFiConfigured && !launchedConfigPortal) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiFailureStartTime == 0) {
        // Start tracking WiFi failure time
        wifiFailureStartTime = millis();
        TLogPlus::Log.debugln("WiFi connection lost - starting failure timer");
      } else if (millis() - wifiFailureStartTime > 60000) {  // 60 seconds
        // WiFi has been disconnected for 60 seconds, launch portal
        TLogPlus::Log.infoln("WiFi disconnected for 60+ seconds - launching configuration portal");
        portalLaunchedManually = false;  // Mark as automatically launched
        startConfigPortal();
      } else if (shouldAttemptWiFiConnection()) {
        // Try to reconnect
        TLogPlus::Log.debugln("Reconnect loop - skipping due to debug.");
        // TLogPlus::Log.debugln("Attempting WiFi reconnection");
        // connectToWiFi();
      }
    } else {
      // WiFi is connected, reset failure timer
      if (wifiFailureStartTime != 0) {
        TLogPlus::Log.debugln("WiFi reconnected - resetting failure timer");
        wifiFailureStartTime = 0;
      }
    }
  }

  // Check if we're in portal mode and should scan for configured network
  if (launchedConfigPortal && !portalLaunchedManually && isWiFiConfigured && 
      screenManager->getScreenMode() == SCREEN_NEEDS_CONFIG) {
    // Only scan for automatically launched portals, and only every 10 seconds
    if (millis() - lastPortalScanTimer > 10000) {
      lastPortalScanTimer = millis();
      
      // Check if the configured network is available
      String configuredSSID = settings->get(SETTING_WIFI_SSID);
      if (!configuredSSID.isEmpty()) {
        int scanResult = WiFi.scanComplete();
        if (scanResult >= 0) {
          // Previous scan completed, check if our network is available
          for (int i = 0; i < scanResult; i++) {
            if (WiFi.SSID(i) == configuredSSID) {
              TLogPlus::Log.infoln("Configured network found in portal mode - attempting to reconnect");
              completeConfigurationPortal();
              break;
            }
          }
          // Start a new scan for next time
          WiFi.scanNetworks(true);
        } else if (scanResult == WIFI_SCAN_FAILED) {
          // No scan running, start one
          WiFi.scanNetworks(true);
        }
        // If scan is running (WIFI_SCAN_RUNNING), just wait for next cycle
      }
    }
  }

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

// Button callback functions
void onButtonRightPress(ButtonPressType type) {
  TLogPlus::Log.printf("Right button press: %u\n", type);
  if (screenManager == nullptr) {
    TLogPlus::Log.debugln("screenManager was null - no button action will occur.");
    return;
  }

  if (type == SHORT_PRESS) {
    screenManager->moveNextScreen(1);
  } else if (type == LONG_PRESS) {
    // Check if we're on the WiFi screen
    if (screenManager->getScreenMode() == SCREEN_WIFI) {
      TLogPlus::Log.infoln("Long press on WiFi screen - starting configuration portal");
      portalLaunchedManually = true;  // Mark as manually launched
      startConfigPortal();
    }
    // Check if we're in portal mode (SCREEN_NEEDS_CONFIG)
    else if (screenManager->getScreenMode() == SCREEN_NEEDS_CONFIG) {
      // Only exit portal if WiFi is configured
      if (isWiFiConfigured) {
        TLogPlus::Log.infoln("Long press in portal mode - exiting portal and reconnecting to WiFi");
        completeConfigurationPortal();
      } else {
        TLogPlus::Log.infoln("Long press in portal mode ignored - no WiFi configured");
      }
    }
  }
}

void onButtonLeftPress(ButtonPressType type) {
  TLogPlus::Log.printf("Left button press: %u\n", type);
  if (screenManager == nullptr) {
    TLogPlus::Log.debugln("screenManager was null - no button action will occur.");
    return;
  }
  if (type == SHORT_PRESS)
    screenManager->moveNextScreen(-1);
}

void startConfigPortal()
{
  launchedConfigPortal = true;
  TLogPlus::Log.infoln("Switching to WiFi AP mode");

  WiFi.mode(WIFI_AP_STA);

  IPAddress apIP(192,168,4,1);        // Default AP IP is 192.168.4.1
  IPAddress gateway(192,168,4,1);
  IPAddress subnet(255,255,255,0);
  WiFi.softAPConfig(apIP, gateway, subnet);
  WiFi.softAP(fullHostname);
  
  dnsServer.start(53, "*", WiFi.softAPIP());

  configureNetworkDependents(true);

  screenManager->setPortalSSID(fullHostname);
  screenManager->setScreenMode(SCREEN_NEEDS_CONFIG);
}

void completeConfigurationPortal()
{
  if (!launchedConfigPortal) {
    return;
  }
  launchedConfigPortal = false;
  portalLaunchedManually = false;  // Reset manual launch flag

  TLogPlus::Log.debugln("Shutting down config portal");
  // wifiManager.stopConfigPortal();

  dnsServer.stop();

  TLogPlus::Log.debugln("Connecting to wifi");
  wifiFailureStartTime = 0;  // Reset failure timer when exiting portal
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
  if(debugCommands.find(cmd.c_str()) != debugCommands.end())
  {
    debugCommands[cmd.c_str()](value);
  } else {
    TLogPlus::Log.printf("Unrecognized debug command: %s\n", cmd);
  }
}

bool connectToWiFi(bool firstAttempt)
{
  String ssid = settings->get(SETTING_WIFI_SSID);
  String password = settings->get(SETTING_WIFI_PSK);

  if (ssid.isEmpty())
  {
    TLogPlus::Log.warningln("No WiFi SSID configured.");
    isWiFiConfigured = false;
    return false;
  }

  isWiFiConfigured = true;
  
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
    WiFi.setAutoReconnect(false);
  }

  uint32_t freeSpace = ESP.getFreeHeap();
  WiFi.mode(WIFI_STA);

  TLogPlus::Log.info("Attempting to connect to WiFi network: ");
  TLogPlus::Log.infoln(ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  return true;
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
  wifiFailureStartTime = 0;  // Reset failure timer when connected
}

void WiFi_GotIPAddress(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.printf("Got IP: %s\n", WiFi.localIP().toString());
  configureNetworkDependents(true);
}

void configureNetworkDependents(bool connected)
{
  if (connected && !networkServicesInitalized)
  {
    networkServicesInitalized = true;
    // We're on the network with an IP address!
    setupTelnetStream();
    if (!webServerManager) {
      webServerManager = new WebServerManager(settings, gpsManager, screenManager);
      webServerManager->setWiFiConnectCallback([](){ connectToWiFi(); });
      webServerManager->begin();
    }
    if (udpManager != nullptr)
      udpManager->begin();
    TLogPlus::Log.println("Network services enabled");
  }
  else if (!connected && networkServicesInitalized)
  {
    // We don't have an IP address or network connection
    networkServicesInitalized = false;
    isTelnetSetup = false;
    telnetSerialStream.stop();
    if (webServerManager) {
      webServerManager->end();
      delete webServerManager;
      webServerManager = nullptr;
    }
    if (udpManager != nullptr)
      udpManager->stop();
    TLogPlus::Log.println("Network services disabled");
  }
}

void WiFi_Disconnected(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info)
{
  TLogPlus::Log.printf("WiFi disconnected; event: %u, reason: %u\n", 
    wifi_event,
    wifi_info.wifi_sta_disconnected.reason);

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

void initDebugCommands()
{
  debugCommands["sendgpscmd"] = [](String value) { gpsManager->sendCommand(value.c_str()); };
  debugCommands["setgpsbaud"] = [](String value) { gpsManager->changeBaud(value.toInt()); };
  debugCommands["setgpsdata"] = [](String value) { gpsManager->setDataMode((GPSDataMode)value.toInt()); };
  debugCommands["setgpsfix"] = [](String value) { gpsManager->setFixRate((GPSRate)value.toInt()); };
  debugCommands["setgpsrate"] = [](String value) { gpsManager->setRefreshRate((GPSRate)value.toInt()); };
  debugCommands["getgps"] = [](String value) { gpsManager->printToLog(); };

  debugCommands["refresh"] = [](String value) { screenManager->refreshScreen(); };
  debugCommands["backlight"] = [](String value) { screenManager->setBacklight((uint8_t)value.toInt()); };
  debugCommands["setscreenmode"] = [](String value) { screenManager->setScreenMode((ScreenMode)value.toInt()); };

  debugCommands["setwifi"] = [](String value) { settings->set(SETTING_WIFI_SSID, value); };
  debugCommands["setpassword"] = [](String value) { settings->set(SETTING_WIFI_PSK, value); };
  debugCommands["getwifi"] = [](String value) { 
    TLogPlus::Log.infoln("Printing WiFi information");
    TLogPlus::Log.printf("Status: %u\n", WiFi.status());
    TLogPlus::Log.printf("IP: %s\n", WiFi.localIP().toString());
    TLogPlus::Log.printf("Base Station ID: %s\n", WiFi.BSSIDstr().c_str());
    TLogPlus::Log.printf("SSID: %s\n", WiFi.SSID().c_str());
    TLogPlus::Log.printf("RSSI: %i\n", WiFi.RSSI()); 
  };
  debugCommands["reconnect"] = [](String value) { connectToWiFi(); };

  debugCommands["setsettings"] = [](String value) { settings->load(value); };
  debugCommands["getsettings"] = [](String value) { settings->printToLog(); };
  debugCommands["reboot"] = [](String value) { ESP.restart(); };

  debugCommands["setudphost"] = [](String value) { 
    udpManager->setDestHost(value.c_str());
    settings->set(SETTING_UDP_HOST, value); 
  };
  debugCommands["setudpport"] = [](String value) { 
    udpManager->setDestPort(value.toInt());
    settings->setInt(SETTING_UDP_PORT, value.toInt());
  };

  debugCommands["help"] = [](String value) {
    TLogPlus::Log.println("Valid debug commands are:");
    for (const auto& pair : debugCommands) {
      TLogPlus::Log.println(pair.first.c_str());
    }
  };
  debugCommands["getlog"] = [](String value) {
    bufferedLogs->printAll(telnetSerialStream);
    bufferedLogs->printAll(Serial);
  };
  debugCommands["getgpsdata"] = [](String value) {
    gpsManager->receivedSentences(telnetSerialStream);
    gpsManager->receivedSentences(Serial);
  };
}