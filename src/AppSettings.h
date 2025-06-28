#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

#define SETTING_SCREEN_REFRESH_INTERVAL "refresh"
#define SETTING_OTA_ENABLED "otaEnable"
#define SETTING_OTA_PASSWORD "otaPassword"
#define SETTING_WIFI_HOSTNAME "hostname"
#define SETTING_AVERAGE_SPEED_WINDOW "avgSpeedWindow"
#define SETTING_DATA_AGE_THRESHOLD "dataAgeThres"
#define SETTING_BAUD_RATE "baud"
#define SETTING_GPS_LOG_ENABLED "gpsLogEnabled"
#define SETTING_GPS_ECHO "gpsEchoEnabled"
#define SETTING_WIFI_SSID "wifiSSID"
#define SETTING_WIFI_PSK "wifiPSK"
#define SETTING_REFRESH_INTERVAL_OTHER "refreshOther"

#define GPS_ECHO_DEFAULT true
#define GPS_LOG_DEFAULT false
#define SCREEN_REFRESH_INTERVAL_DEFAULT 5000
#define OTA_ENABLED_DEFAULT true
#define OTA_PASSWORD_DEFAULT "password"
#define WIFI_HOSTNAME_DEFAULT "GPS_S3"
#define SPEED_AVG_WINDOW_DEFAULT 10
#define MAX_COMMAND_LEN 120
#define DATA_AGE_THRESHOLD_DEFAULT 5000 // milliseconds
#define AVG_SPEED_WINDOW_DEFAULT 10
#define DATA_AGE_DEFAULT 5000
#define BAUD_RATE_DEFAULT 9600


class AppSettings {
public:
    AppSettings();
    AppSettings(FS* fileSystem, const char* filename = "/settings.json");
    void loadDefaults();
    bool load();
    bool load(String json);
    bool save();
    void set(const char* key, const char* value);
    void set(const char* key, String value);
    void setBool(const char* key, bool value);
    void setInt(const char* key, int value);
    void setFloat(const char* key, float value);
    String get(const char* key, const char* defaultValue = "");
    bool getBool(const char* key, bool defaultValue = false);
    int getInt(const char* key, int defaultValue = 0);
    float getFloat(const char* key, float defaultValue = 0.0f);

private:
    String _filename;
    FS* _fileSystem;
    JsonDocument _doc;
};

#endif // APP_SETTINGS_H
