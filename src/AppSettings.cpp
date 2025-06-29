#include "AppSettings.h"
#include <TLogPlus.h>

AppSettings::AppSettings() :
    _fileSystem(nullptr)
{
    _doc = JsonDocument();
    // Constructor for a non-FS app settings instance
    TLogPlus::Log.warningln("AppSettings is running in memory-only mode. Changes will not be persisted.");

}

AppSettings::AppSettings(FS* fileSystem, const char* filename) : 
    _filename(filename), _fileSystem(fileSystem) {
    // Constructor
}

bool AppSettings::load() {
    if (_fileSystem == nullptr) {
        return true;
    }

    File file = LittleFS.open(_filename.c_str(), "r");
    if (!file) {
        return false;
    }
    DeserializationError error = deserializeJson(_doc, file);
    file.close();
    return !error;
}

bool AppSettings::load(String json)
{
    JsonDocument newSettings;
    newSettings.clear();
    DeserializationError error = deserializeJson(newSettings, json);

    if (!error)
    {
        _doc.clear();
        _doc = newSettings;
    }
    else
    {
        TLogPlus::Log.printf("AppSettings load error: %s", error.c_str());
    }
    return !error;
}

void AppSettings::loadDefaults() {
    _doc.clear();
    setBool(SETTING_GPS_ECHO, GPS_ECHO_DEFAULT);
    setBool(SETTING_GPS_LOG_ENABLED, GPS_LOG_DEFAULT);
    setBool(SETTING_OTA_ENABLED, OTA_ENABLED_DEFAULT);
    set(SETTING_OTA_PASSWORD, OTA_PASSWORD_DEFAULT);
    setInt(SETTING_AVERAGE_SPEED_WINDOW, AVG_SPEED_WINDOW_DEFAULT);
    setInt(SETTING_DATA_AGE_THRESHOLD, DATA_AGE_DEFAULT);
    setInt(SETTING_BAUD_RATE, BAUD_RATE_DEFAULT);
    setInt(SETTING_SCREEN_REFRESH_INTERVAL, SCREEN_REFRESH_INTERVAL_DEFAULT);
    setInt(SETTING_REFRESH_INTERVAL_OTHER, REFRESH_INTERVAL_OTHER_DEFAULT);
    save();
}

bool AppSettings::save() {
    if (_fileSystem == nullptr)
    {
        TLogPlus::Log.warningln("AppSettings is running in memory-only mode. Changes will not be persisted.");
        return false;
    }
    
    TLogPlus::Log.infoln("Saving settings to file: " + _filename);
    File file = LittleFS.open(_filename.c_str(), "w");
    if (!file) {
        return false;
    }
    bool success = (serializeJson(_doc, file) > 0);
    TLogPlus::Log.infoln("Finished saving: " + success ? "success" : "failure");
    file.close();
    return success;
}

void AppSettings::set(const char* key, const char* value) {
    _doc[key] = value;
}

void AppSettings::set(const char* key, String value) 
{
    _doc[key] = value;
}

void AppSettings::setBool(const char* key, bool value) {
    _doc[key] = value;
}

void AppSettings::setInt(const char* key, int value) {
    _doc[key] = value;
}

void AppSettings::setFloat(const char* key, float value) {
    _doc[key] = value;
}

String AppSettings::get(const char* key, const char* defaultValue) {
    if (_doc[key].is<String>())
    {
        return _doc[key].as<String>();
    }
    return String(defaultValue);
}

bool AppSettings::getBool(const char* key, bool defaultValue) {
    if (_doc[key].is<bool>()) {
        return _doc[key].as<bool>();
    }
    return defaultValue;
}

int AppSettings::getInt(const char* key, int defaultValue) {
    if (_doc[key].is<int>()) {
        return _doc[key].as<int>();
    }
    return defaultValue;
}

float AppSettings::getFloat(const char* key, float defaultValue) {
    if (_doc[key].is<float>()) {
        return _doc[key].as<float>();
    }
    return defaultValue;
}

void AppSettings::printToLog() {
    TLogPlus::Log.infoln("AppSettings:");
    for (JsonPair kv : _doc.as<JsonObject>()) {
        String value = kv.value().is<String>() ? kv.value().as<String>() : String(kv.value().as<int>());
        TLogPlus::Log.printf("%s: %s\n", kv.key().c_str(), value.c_str());
    }
}

String AppSettings::getRawJson() {
    String result;
    serializeJson(_doc, result);
    return result;
}
