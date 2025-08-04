#pragma once

#define WIFI_RECONNECT_TIMEOUT 10000

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
#define I2C_SDA 43
#define I2C_SCL 44
#define I2C_FREQ 400000
#endif

#if DEV_DEVICE == UNEXPECTED_MAKER_FEATHER_S3
// FeatherS3 pinouts: https://esp32s3.com/feathers3.html
// Ultimate GPS featherwing: https://learn.adafruit.com/adafruit-ultimate-gps-featherwing/pinouts
#define GPS_RX_PIN 44       // TX / D1 / IO43 
#define GPS_TX_PIN 43       // RX / D0 / IO44
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_FREQ 400000

// OLED display and buttons: https://learn.adafruit.com/adafruit-128x64-oled-featherwing/arduino-code
// for ESP32-S2 or S3 feathers
#define BTN_RIGHT_PIN 15 // A
// #define BTN_MIDDLE_PIN 32 // B
#define BTN_LEFT_PIN  14 // C
#define GYRO_ENABLED 1
#endif


#ifndef AUTO_VERSION
#define AUTO_VERSION "version-missing"
#endif