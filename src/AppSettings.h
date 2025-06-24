#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

class AppSettings {
public:
    AppSettings(FS* fileSystem, const char* filename = "/settings.json");
    bool load();
    bool save();
    void set(const char* key, const char* value);
    void set(const char* key, String value);
    void setBool(const char* key, bool value);
    void setInt(const char* key, int value);
    void setFloat(const char* key, float value);
    const char* get(const char* key, const char* defaultValue = "");
    bool getBool(const char* key, bool defaultValue = false);
    int getInt(const char* key, int defaultValue = 0);
    float getFloat(const char* key, float defaultValue = 0.0f);

private:
    const char* _filename;
    FS* _fileSystem;
    JsonDocument _doc;
};

#endif // APP_SETTINGS_H
