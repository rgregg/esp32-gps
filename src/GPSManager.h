#pragma once
#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include <Arduino.h>

enum GPSRate
{
    UPDATE_1_HERTZ = 1,             // Every second
    UPDATE_2_HERTZ = 2,             // 2x per second
    UPDATE_5_HERTZ = 5,             // 5x per second
    UPDATE_10_HERTZ = 10,            // 10x per second
    UPDATE_100_MILLIHERTZ = 100,      // Every 10 seconds
    UPDATE_200_MILLIHERTZ = 200,      // Every 5 seconds
};

enum GPSDataMode
{
    RMC_ONLY = 0,
    GLL_ONLY = 1,
    VTG_ONLY = 2,
    GGA_ONLY = 3,
    GSA_ONLY = 4,
    GSV_ONLY = 5,
    RMC_GGA = 6,
    RMC_GGA_GSA = 7,
    ALL_DATA = 8,
    NO_DATA = -1
};

class GPSManager {
public:
    GPSManager(HardwareSerial* serial, uint32_t rxPin, uint32_t txPin, uint32_t baudRate, bool echoToLog, uint32_t dataAge);
    void begin();
    void loop();
    bool isDataOld() const;
    void updateSpeedAverage(float newSpeed);
    float getSpeedAverage() const;
    void updateLatestData();
    Adafruit_GPS& getGPS();
    uint32_t getLastDataReceivedTime() const;
    void sendCommand(const char* sentence);
    void changeBaud(uint32_t newBaudRate);
    void setRefreshRate(GPSRate rate);
    void setFixRate(GPSRate rate);
    void setDataMode(GPSDataMode mode);
    void setSerialBatchRead(bool readAllTogether = true);

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
    bool _hasBegun = false;
    bool _serialBatchRead = false;

    String formatDMS(float value, char type);

    String _timeStr, _dateStr, _fixStr, _locationStr, _speedStr, _angleStr, _altitudeStr, _satellitesStr, _antennaStr;
    bool _hasFix;
    bool _echoToLog;
    uint32_t _dataAgeThreshold;

};
