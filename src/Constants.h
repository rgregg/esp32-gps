#pragma once

#define GPSECHO true
#define GPSLogToSerial false
#define SCREEN_REFRESH_INTERVAL 5000
#define OTA_ENABLED true
#define OTA_PASSWORD "password"
#define WIFI_HOSTNAME "S3_GPSReceiver"
#define SPEED_AVG_WINDOW 10
#define MAX_COMMAND_LEN 120
#define DATA_AGE_THRESHOLD 5000 // milliseconds

#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3
#define GFX_EXTRA_PRE_INIT()              \
    {                                     \
        pinMode(15 /* PWD */, OUTPUT);    \
        digitalWrite(15 /* PWD */, HIGH); \
    }
#define GFX_BL 38


enum BaudRate {
  BAUD_9600   = 9600,
  BAUD_57600   = 57600,
  BAUD_115200 = 115200
};

enum ScreenMode {
  ScreenMode_BOOT = 0,
  ScreenMode_GPS,
  ScreenMode_OTA,
  ScreenMode_PORTAL
};
