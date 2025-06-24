#include "AppSettings.h"

AppSettings::AppSettings(FS* fileSystem, const char* filename) : 
    _filename(filename), _fileSystem(fileSystem) {
    // Constructor
}

bool AppSettings::load() {
    File file = SPIFFS.open(_filename, "r");
    if (!file) {
        return false;
    }
    DeserializationError error = deserializeJson(_doc, file);
    file.close();
    return !error;
}

bool AppSettings::save() {
    File file = SPIFFS.open(_filename, "w");
    if (!file) {
        return false;
    }
    bool success = (serializeJson(_doc, file) > 0);
    file.close();
    return success;
}

void AppSettings::set(const char* key, const char* value) {
    _doc[key] = value;
}

void AppSettings::set(const char* key, String value) 
{
    _doc[key] = value.c_str();
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

const char* AppSettings::get(const char* key, const char* defaultValue) {
    if (_doc[key].is<String>())
    {
        return _doc[key].as<String>().c_str();
    }
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
