#pragma once
#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include <Arduino.h>

class GPSManager {
public:
    GPSManager(HardwareSerial* serial, uint32_t rxPin, uint32_t txPin, uint32_t baudRate = 9600, bool echoToLog = false);
    void begin();
    void loop(bool batchRead = true);
    bool isDataOld() const;
    void updateSpeedAverage(float newSpeed);
    float getSpeedAverage() const;
    void updateLatestData();
    Adafruit_GPS& getGPS();
    uint32_t getLastDataReceivedTime() const;

    String getTimeStr() const { return _timeStr; }
    String getDateStr() const { return _dateStr; }
    String getFixStr() const { return _fixStr; }
    String getLocationStr() const { return _locationStr; }
    String getSpeedStr() const { return _speedStr; }
    String getAngleStr() const { return _angleStr; }
    String getAltitudeStr() const { return _altitudeStr; }
    String getSatellitesStr() const { return _satellitesStr; }
    String getAntennaStr() const { return _antennaStr; }
    bool hasFix() const { return _hasFix; }

private:
    HardwareSerial* _serial;
    Adafruit_GPS _gps;
    uint32_t _rxPin, _txPin, _baudRate;
    uint32_t _lastDataReceivedTimer = 0;
    float _speedBuffer[10] = {0};
    int _speedIndex = 0;
    int _speedCount = 0;
    float _speedSum = 0;

    String formatDMS(float value, char type);

    String _timeStr, _dateStr, _fixStr, _locationStr, _speedStr, _angleStr, _altitudeStr, _satellitesStr, _antennaStr;
    bool _hasFix;
    bool _echoToLog;

};
