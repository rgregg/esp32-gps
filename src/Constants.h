#pragma once

#define WIFI_RECONNECT_TIMEOUT 5000
#define ENABLE_TELNET true

// These define the device configurations we support
#define LILYGO_T_DISPLAY_S3 1
#define UNEXPECTED_MAKER_FEATHER_S3 2
// These constants are used in the platformio.ini file to set DEV_DEVICE

#if DEV_DEVICE == LILYGO_T_DISPLAY_S3
#define GPS_RX_PIN 18
#define GPS_TX_PIN 21       // Should really be 17 but I connected to the wrong pin
#define BTN_RIGHT_PIN 14
#define BTN_MIDDLE_PIN -1   // No middle button
#define BTN_LEFT_PIN  0
#define GYRO_ENABLED 0
#endif

#if DEV_DEVICE == UNEXPECTED_MAKER_FEATHER_S3
// FeatherS3 pinouts: https://esp32s3.com/feathers3.html
// Ultimate GPS featherwing: https://learn.adafruit.com/adafruit-ultimate-gps-featherwing/pinouts
#define GPS_RX_PIN 43       // TX / D1 / IO43 
#define GPS_TX_PIN 44       // RX / D0 / IO44

// OLED display and buttons: https://learn.adafruit.com/adafruit-128x64-oled-featherwing/arduino-code
// for ESP32-S2 or S3 feathers
#define BTN_RIGHT_PIN 9 // A
#define BTN_MIDDLE_PIN 6 // B
#define BTN_LEFT_PIN  5 // C
#define GYRO_ENABLED 1
#endif


#ifndef AUTO_VERSION
#define AUTO_VERSION "version-missing"
#endif