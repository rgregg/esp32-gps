#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <Preferences.h>

#define AVG_SPEED_WINDOW_DEFAULT 10
#define BAUD_RATE_DEFAULT 9600
#define DATA_AGE_DEFAULT 5000
#define DATA_AGE_THRESHOLD_DEFAULT 5000 // milliseconds
#define GPS_DATA_MODE_DEFAULT 6  // RMC_GGA
#define GPS_FIX_RATE_DEFAULT 1   // UPDATE_1_HERTZ
#define GPS_LOG_DEFAULT false
#define GPS_UPDATE_RATE_DEFAULT 1  // UPDATE_1_HERTZ
#define MAX_COMMAND_LEN 120
#define REFRESH_INTERVAL_OTHER_DEFAULT 5000
#define SCREEN_REFRESH_INTERVAL_DEFAULT 5000
#define BACKLIGHT_DEFAULT 100
#define UDP_ENABLED_DEFAULT false
#define UDP_HOST_DEFAULT ""
#define UDP_PORT_DEFAULT 10110
#define DISPLAY_ROTATION_DEFAULT 1

#define SETTING_AVERAGE_SPEED_WINDOW "avgSpeedWindow"
#define SETTING_BAUD_RATE "baud"
#define SETTING_DATA_AGE_THRESHOLD "dataAgeThres"
#define SETTING_GPS_DATA_MODE "gpsDataMode"
#define SETTING_GPS_FIX_RATE "gpsFixRate"
#define SETTING_GPS_LOG_ENABLED "gpsLogEnabled"
#define SETTING_GPS_UPDATE_RATE "gpsUpdateRate"
#define SETTING_REFRESH_INTERVAL_OTHER "refreshOther"
#define SETTING_SCREEN_REFRESH_INTERVAL "refresh"
#define SETTING_BACKLIGHT "backlight"
#define SETTING_UDP_ENABLED "udpEnabled"
#define SETTING_UDP_HOST "udpHost"
#define SETTING_UDP_PORT "udpPort"
#define SETTING_DISPLAY_ROTATION "displayRotation"
#define SETTING_WIFI_HOSTNAME "hostname"
#define SETTING_WIFI_PSK "wifiPSK"
#define SETTING_WIFI_SSID "wifiSSID"
#define SPEED_AVG_WINDOW_DEFAULT 10
#define WIFI_HOSTNAME_DEFAULT "Nomaduino"
#define USE_MAGNETOMETER "use_magnetometer"
#define USE_DISPLAY "use_display_2"
#define USE_SERIAL_GPS "use_serial_gps"
#define USE_BUTTONS "use_buttons_2"

#define SETTING_MAG_CALIBRATION_MODE_ENABLED "magCalEnabled"
#define SETTING_MAG_OFFSET_X "magOffsetX"
#define SETTING_MAG_OFFSET_Y "magOffsetY"
#define SETTING_MAG_OFFSET_Z "magOffsetZ"

#define MAG_CALIBRATION_MODE_DEFAULT false
#define MAG_OFFSET_X_DEFAULT 0.0f
#define MAG_OFFSET_Y_DEFAULT 0.0f
#define MAG_OFFSET_Z_DEFAULT 0.0f


class AppSettings {
public:
    AppSettings();
    void loadDefaults();
    bool load();
    bool load(String json);

    void set(const char* key, const char* value);
    void set(const char* key, String value);
    void setBool(const char* key, bool value);
    void setInt(const char* key, int value);
    void setFloat(const char* key, float value);
    
    String get(const char* key, const char* defaultValue = "");
    bool getBool(const char* key, bool defaultValue = false);
    int getInt(const char* key, int defaultValue = 0);
    float getFloat(const char* key, float defaultValue = 0.0f);
    
    void printToLog();
    String getRawJson();
private:
    Preferences _prefs;
};

#endif // APP_SETTINGS_H
