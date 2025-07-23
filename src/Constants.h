#pragma once

#define WIFI_RECONNECT_TIMEOUT 5000
#define ENABLE_TELNET true
#define GPS_RX_PIN 18
#define GPS_TX_PIN 21       // Should really be 17 but I connected to the wrong pin

#define BTN_RIGHT_PIN 14
#define BTN_LEFT_PIN  0

#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3
#define GFX_BL 38     // backlight pin
#define SCREEN_POWER 15  // screen power pin
#define SCREEN_DC_PIN 7
#define SCREEN_CS_PIN 6
#define SCREEN_WR_PIN 8
#define SCREEN_RD_PIN 9
#define SCREEN_D0_PIN 39
#define SCREEN_D1_PIN 40
#define SCREEN_D2_PIN 41
#define SCREEN_D3_PIN 42
#define SCREEN_D4_PIN 45
#define SCREEN_D5_PIN 46
#define SCREEN_D6_PIN 47
#define SCREEN_D7_PIN 48
#define SCREEN_RST_PIN 5
#define SCREEN_ROTATION 0
#define SCREEN_WIDTH 170
#define SCREEN_HEIGHT 320
#define SCREEN_IPS true
#define SCREEN_COL_OFFSET 35
#define SCREEN_ROW_OFFSET 0


#define SH1107_SDA 21
#define SH1107_SCL 22
#define SH1107_RST -1 // No reset pin for SH1107
#define SH1107_WIDTH 128
#define SH1107_HEIGHT 64
#define SH1107_ROTATION 0
#define SH1107_BL 38
#define SH1107_POWER 15

#ifndef AUTO_VERSION
#define AUTO_VERSION "0.0.0"
#endif