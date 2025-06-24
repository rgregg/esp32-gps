#include "GPSManager.h"
#include <TLog.h> 
#include "Constants.h"

GPSManager::GPSManager(HardwareSerial* serial, uint32_t rxPin, uint32_t txPin, uint32_t baudRate, bool echoToLog)
    : _serial(serial), _gps(serial), _rxPin(rxPin), _txPin(txPin), _baudRate(baudRate), _echoToLog(echoToLog) {
      // check baud rate is valid
      if (!(baudRate == 9600 || baudRate == 57600 || baudRate == 115200))
      {
        Log.println("GPS: invalid baud rate: " + String(baudRate));
      }
    }

void GPSManager::begin() {
    // start at 9600 baud as the default
    _serial->begin(9600, SERIAL_8N1, _rxPin, _txPin, false);
    _gps.begin(9600);
    if (_baudRate != 9600) {
        switch(_baudRate) {
            case 57600:
                _gps.sendCommand(PMTK_SET_BAUD_57600);
                delay(100);
                _gps.begin(_baudRate);
                break;
            case 115200:
                _gps.sendCommand(PMTK_SET_BAUD_115200);
                delay(100);
                _gps.begin(_baudRate);
                break;
            default:
              Log.println("WARNING: Invalid baud rate for GPS. Will use the default rate.");
              break;
        }
        
    }
    _gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    _gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    _gps.sendCommand(PGCMD_ANTENNA);
    delay(1000);
    _gps.sendCommand(PMTK_Q_RELEASE);
}

void GPSManager::loop(bool batchRead) {

    // Read from the serial connection
    if (batchRead)
    {
      while(_gps.available() > 0)
      {
        char c = _gps.read();
        if (_echoToLog) Log.print(c);
      }
    } else {
      char c = _gps.read();
      if (_echoToLog) Log.print(c);
    }

    // Check to see if anything new arrived
    if (_gps.newNMEAreceived()) {
        if (!_gps.parse(_gps.lastNMEA())) {
            // Ignore bad data
            return;
        }
        _lastDataReceivedTimer = millis();
        updateLatestData();
    }
}

bool GPSManager::isDataOld() const {
    if (_lastDataReceivedTimer == 0) return true;
    return (millis() - _lastDataReceivedTimer > DATA_AGE_THRESHOLD);
}

void GPSManager::updateSpeedAverage(float newSpeed) {
    _speedSum -= _speedBuffer[_speedIndex];
    _speedBuffer[_speedIndex] = newSpeed;
    _speedSum += newSpeed;
    _speedIndex = (_speedIndex + 1) % 10;
    if (_speedCount < 10) _speedCount++;
}

float GPSManager::getSpeedAverage() const {
    if (_speedCount == 0) return 0;
    return _speedSum / _speedCount;
}

void GPSManager::updateLatestData() {
    _hasFix = (_gps.fix == 1);
    // Format time
    String timeStr = "";
    if (_gps.hour < 10) timeStr += '0';
    timeStr += String(_gps.hour) + ':';
    if (_gps.minute < 10) timeStr += '0';
    timeStr += String(_gps.minute) + ':';
    if (_gps.seconds < 10) timeStr += '0';
    timeStr += String(_gps.seconds); //+ '.';
    // if (GPS.milliseconds < 10) timeStr += "00";
    // else if (GPS.milliseconds < 100) timeStr += "0";
    // timeStr += String(GPS.milliseconds);
    _timeStr = timeStr;

    // Format date
    _dateStr = String(_gps.month) + "/" + String(_gps.day) + "/20" + String(_gps.year);

    // Fix and quality
    _fixStr = "Fix: " + String((int)_gps.fix) + " quality: " + String((int)_gps.fixquality);

    // Location and other GPS data if fix is valid
    _satellitesStr = "Satellites: " + String((int)_gps.satellites);
    _antennaStr    = "Antenna: " + String((int)_gps.antenna);

    if (_hasFix) {
      String locationStr = formatDMS(_gps.latitude, _gps.lat);
      locationStr += "\n";
      locationStr += formatDMS(_gps.longitude, _gps.lon);
      _locationStr = locationStr;
      
      updateSpeedAverage(_gps.speed);
      _speedStr      = "Speed (knots): " + String(_gps.speed) + " (Avg: " + String(getSpeedAverage()) + ")";

      _angleStr      = "Angle: " + String(_gps.angle);
      _altitudeStr   = "Altitude: " + String(_gps.altitude);
      
    } else {
      _locationStr = "No Fix";
      _speedStr = _angleStr = _altitudeStr = "";
    }
}

String GPSManager::formatDMS(float raw, char dir) {
  int degrees = (dir == 'N' || dir == 'S') ? int(raw) / 100 : int(raw) / 100; // same for both formats
  float minutesFloat = raw - (degrees * 100);
  int minutes = int(minutesFloat);
  float seconds = (minutesFloat - minutes) * 60;

  String dms = String(degrees) + "o" + String(minutes) + "'" + String(seconds, 2) + "\" " + dir;
  return dms;
}

Adafruit_GPS& GPSManager::getGPS() { return _gps; }

uint32_t GPSManager::getLastDataReceivedTime() const { return _lastDataReceivedTimer; }
