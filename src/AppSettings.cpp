#include "AppSettings.h"
#include <TLogPlus.h>
#include <ArduinoJson.h>

#define SETTING_IS_CONFIGURED "hasSetup"

AppSettings::AppSettings() {
    // Default constructor
}

bool AppSettings::load() {
    _prefs.begin("esp32_gps", false);

    return _prefs.getBool(SETTING_IS_CONFIGURED, false);
}

bool AppSettings::load(String json)
{
    JsonDocument newSettings;
    DeserializationError error = deserializeJson(newSettings, json);

    if (!error)
    {
        for (JsonPair kv : newSettings.as<JsonObject>()) {
            if (kv.value().is<bool>()) {
                _prefs.putBool(kv.key().c_str(), kv.value().as<bool>());
            } else if (kv.value().is<int>()) {
                _prefs.putInt(kv.key().c_str(), kv.value().as<int>());
            } else if (kv.value().is<float>()) {
                _prefs.putFloat(kv.key().c_str(), kv.value().as<float>());
            } else if (kv.value().is<String>()) {
                _prefs.putString(kv.key().c_str(), kv.value().as<String>());
            }
        }
    }
    else
    {
        TLogPlus::Log.printf("AppSettings load error: %s", error.c_str());
    }
    return !error;
}

void AppSettings::loadDefaults() {
    _prefs.clear();
    setBool(SETTING_GPS_ECHO, GPS_ECHO_DEFAULT);
    setBool(SETTING_GPS_LOG_ENABLED, GPS_LOG_DEFAULT);
    setInt(SETTING_GPS_DATA_MODE, GPS_DATA_MODE_DEFAULT);
    setInt(SETTING_GPS_FIX_RATE, GPS_FIX_RATE_DEFAULT);
    setInt(SETTING_GPS_UPDATE_RATE, GPS_UPDATE_RATE_DEFAULT);
    setInt(SETTING_AVERAGE_SPEED_WINDOW, AVG_SPEED_WINDOW_DEFAULT);
    setInt(SETTING_DATA_AGE_THRESHOLD, DATA_AGE_DEFAULT);
    setInt(SETTING_BAUD_RATE, BAUD_RATE_DEFAULT);
    setInt(SETTING_SCREEN_REFRESH_INTERVAL, SCREEN_REFRESH_INTERVAL_DEFAULT);
    setInt(SETTING_REFRESH_INTERVAL_OTHER, REFRESH_INTERVAL_OTHER_DEFAULT);
    setInt(SETTING_BACKLIGHT, BACKLIGHT_DEFAULT);
    setBool(SETTING_UDP_ENABLED, UDP_ENABLED_DEFAULT);
    set(SETTING_UDP_HOST, UDP_HOST_DEFAULT);
    setInt(SETTING_UDP_PORT, UDP_PORT_DEFAULT);
    setBool(SETTING_IS_CONFIGURED, true);
    setInt(SETTING_DISPLAY_ROTATION, DISPLAY_ROTATION_DEFAULT);
}

void AppSettings::set(const char* key, const char* value) {
    _prefs.putString(key, value);
}

void AppSettings::set(const char* key, String value) 
{
    _prefs.putString(key, value);
}

void AppSettings::setBool(const char* key, bool value) {
    _prefs.putBool(key, value);
}

void AppSettings::setInt(const char* key, int value) {
    _prefs.putInt(key, value);
}

void AppSettings::setFloat(const char* key, float value) {
    _prefs.putFloat(key, value);
}

String AppSettings::get(const char* key, const char* defaultValue) {
    return _prefs.getString(key, String(defaultValue));
}

bool AppSettings::getBool(const char* key, bool defaultValue) {
    return _prefs.getBool(key, defaultValue);
}

int AppSettings::getInt(const char* key, int defaultValue) {
    return _prefs.getInt(key, defaultValue);
}

float AppSettings::getFloat(const char* key, float defaultValue) {
    return _prefs.getFloat(key, defaultValue);
}

void AppSettings::printToLog() {
    TLogPlus::Log.infoln("AppSettings:");
    TLogPlus::Log.println(getRawJson());
}

String AppSettings::getRawJson() {
    JsonDocument doc;
    doc[SETTING_AVERAGE_SPEED_WINDOW] = getInt(SETTING_AVERAGE_SPEED_WINDOW);
    doc[SETTING_BAUD_RATE] = getInt(SETTING_BAUD_RATE);
    doc[SETTING_DATA_AGE_THRESHOLD] = getInt(SETTING_DATA_AGE_THRESHOLD);
    doc[SETTING_GPS_DATA_MODE] = getInt(SETTING_GPS_DATA_MODE);
    doc[SETTING_GPS_ECHO] = getBool(SETTING_GPS_ECHO);
    doc[SETTING_GPS_FIX_RATE] = getInt(SETTING_GPS_FIX_RATE);
    doc[SETTING_GPS_LOG_ENABLED] = getBool(SETTING_GPS_LOG_ENABLED);
    doc[SETTING_GPS_UPDATE_RATE] = getInt(SETTING_GPS_UPDATE_RATE);
    doc[SETTING_REFRESH_INTERVAL_OTHER] = getInt(SETTING_REFRESH_INTERVAL_OTHER);
    doc[SETTING_SCREEN_REFRESH_INTERVAL] = getInt(SETTING_SCREEN_REFRESH_INTERVAL);
    doc[SETTING_BACKLIGHT] = getInt(SETTING_BACKLIGHT);
    doc[SETTING_WIFI_HOSTNAME] = get(SETTING_WIFI_HOSTNAME);
    doc[SETTING_WIFI_SSID] = get(SETTING_WIFI_SSID);
    doc[SETTING_WIFI_PSK] = get(SETTING_WIFI_PSK);
    doc[SETTING_UDP_ENABLED] = getBool(SETTING_UDP_ENABLED);
    doc[SETTING_UDP_HOST] = get(SETTING_UDP_HOST);
    doc[SETTING_UDP_PORT] = getInt(SETTING_UDP_PORT);
    doc[SETTING_DISPLAY_ROTATION] = getInt(SETTING_DISPLAY_ROTATION);

    String json;
    serializeJsonPretty(doc, json);
    return json;
}
