#include "GPSManager.h"
#include "Constants.h"
#include <TLogPlus.h> 

GPSManager::GPSManager(HardwareSerial* serial, uint32_t rxPin, uint32_t txPin, uint32_t baudRate, bool echoToLog, uint32_t dataAge, GPSDataMode dataMode, GPSRate fixRate, GPSRate updateRate)
    : _serial(serial), _gps(serial), _rxPin(rxPin), _txPin(txPin), _baudRate(baudRate), _echoToLog(echoToLog), _dataAgeThreshold(dataAge), _dataMode(dataMode), _fixRate(fixRate), _updateRate(updateRate) {
      // check baud rate is valid
      if (!(baudRate == 9600 || baudRate == 57600 || baudRate == 115200))
      {
        TLogPlus::Log.printf("GPS: unsupported baud rate: %u", baudRate);
      }
      _hasFix = false;
}

void GPSManager::begin() {
    // start at 9600 baud as the default
    _serial->begin(9600, SERIAL_8N1, _rxPin, _txPin, false);
    _gps.begin(9600);
    _hasBegun = true;
    changeBaud(_baudRate);
    delay(100);
    setDataMode(_dataMode);
    setFixRate(_fixRate);
    setRefreshRate(_updateRate);
    _gps.sendCommand(PGCMD_ANTENNA);
    delay(500);
    _gps.sendCommand(PMTK_Q_RELEASE);
}

void GPSManager::loop() {
    // Read from the serial connection and echo to the logs if enabled
    if (_serialBatchRead)
    {
        while(_gps.available() > 0)
        {
            char c = _gps.read();
            if (_echoToLog) 
            {
                TLogPlus::Log.debug("%c", c);
            }
        }
    } else {
        char c = _gps.read();
        if (_echoToLog)
        {
            TLogPlus::Log.debug("%c", c);
        }
    }

    // Check to see if anything new arrived
    if (_gps.newNMEAreceived()) {
      char* lastSentence = _gps.lastNMEA();
      if (!_gps.parse(lastSentence)) {
          // Ignore bad data
          return;
      }

      // Send via UDP to remote listener (if enabled)
      if (_udpManager != nullptr) {
            _udpManager->send(lastSentence);
      }

      // <eep track of the last time we got an update
      _lastDataReceivedTimer = millis();
      updateLatestData();
    }
}

void GPSManager::setUDPManager(UDPManager* udpManager) {
    _udpManager = udpManager;
}

bool GPSManager::isDataOld() const {
    if (_lastDataReceivedTimer == 0) return true;
    return (millis() - _lastDataReceivedTimer > _dataAgeThreshold);
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
    timeStr += String(_gps.seconds);
    _timeStr = timeStr;

    // Format date
    _dateStr = String(_gps.month) + "/" + String(_gps.day) + "/20" + String(_gps.year);

    // Fix and quality
    switch(_gps.fixquality)
    {
      case 0:   // Invalid
        _fixStr = "No fix";
        break;
      case 1: // GPS Fix
        _fixStr = "GPS fix";
        break;
      case 2: // DGPS Fix
        _fixStr = "Differential GPS fix";
        break;
      default:
        _fixStr = "Unknown Value: " + String(_gps.fixquality);
        break;
    }
    switch(_gps.fixquality_3d)
    {
      case 1: // No Fix
        break;      
      case 2: // 2D Fix
        _fixStr += " (2D)";
        break;
      case 3: // 3D Fix
        _fixStr += " (3D)";
        break;
    }

    // _fixStr = "Fix: " + String((int)_gps.fix) + " quality: " + String((int)_gps.fixquality);

    // Location and other GPS data if fix is valid
    _satellitesStr = "Satellites: " + String((int)_gps.satellites);
    _antennaStr    = "Antenna: " + String((int)_gps.antenna);

    if (_hasFix) {

      String locationStr = formatDMS(getDMS(_gps.fix, _gps.latitude, _gps.lat));
      locationStr += "\n";
      locationStr += formatDMS(getDMS(_gps.fix, _gps.longitude, _gps.lon));
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

DMS GPSManager::getLatitude() {
  return getDMS(_gps.fix, _gps.latitude, _gps.lat);
}

DMS GPSManager::getLongitude() {
  return getDMS(_gps.fix, _gps.longitude, _gps.lon);
}

int GPSManager::getDirectionFromTrueNorth() {
  return int(_gps.angle);
}

float GPSManager::getSpeed() {
  return _gps.speed;
}

DMS GPSManager::getDMS(bool fix, float raw, char dir) {
  DMS result;
  if (!fix) 
  {
    result.hasValue = false;
  }
  else
  {
    result.direction = dir;
    result.rawValue = raw;
    result.degrees = int(raw) / 100;
    float minutesFloat = raw - (result.degrees * 100);
    result.minutes = int(minutesFloat);
    result.seconds = (minutesFloat - result.minutes) * 60;
    result.hasValue = true;
  }
  return result;
}

String GPSManager::formatDMS(DMS data) {
  String dms = String(data.degrees) + "\xB0" + String(data.minutes) + "'" + String(data.seconds, 2) + "\" " + data.direction;
  return dms;
}

void GPSManager::sendCommand(const char* sentence)
{
  if (!_hasBegun)
  {
    TLogPlus::Log.warningln("GPS: sending command before serial connection established.");
  }
  _gps.sendCommand(sentence);
}


void GPSManager::changeBaud(uint32_t baudRate)
{
  switch(baudRate) {
    case 9600:
      _gps.sendCommand(PMTK_SET_BAUD_9600);
      break;
    case 57600:
      _gps.sendCommand(PMTK_SET_BAUD_57600);
      break;
    case 115200:
      _gps.sendCommand(PMTK_SET_BAUD_115200);
      break;
    default:
      TLogPlus::Log.warningln("GPS: Invalid baud rate. Will use the default rate of 9600.");
      break;
  }

  delay(100);
  _gps.begin(baudRate);
}

void GPSManager::setRefreshRate(GPSRate rate)
{
  switch(rate) {
    case UPDATE_1_HERTZ:
      sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
      break;
    case UPDATE_2_HERTZ:
      sendCommand(PMTK_SET_NMEA_UPDATE_2HZ);
      break;
    case UPDATE_5_HERTZ:
      sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
      break;
    case UPDATE_10_HERTZ:
      sendCommand(PMTK_SET_NMEA_UPDATE_10HZ);
      break;
    case UPDATE_100_MILLIHERTZ:
      sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ);
      break;
    case UPDATE_200_MILLIHERTZ:
      sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ);
      break;
    default:
      TLogPlus::Log.warning("Unsupported refresh rate.");
      break;
  }
}

void GPSManager::setFixRate(GPSRate rate)
{
  switch(rate) {
    case UPDATE_1_HERTZ:
      sendCommand(PMTK_API_SET_FIX_CTL_1HZ);
      break;
    case UPDATE_5_HERTZ:
      sendCommand(PMTK_API_SET_FIX_CTL_5HZ);
      break;
    case UPDATE_100_MILLIHERTZ:
      sendCommand(PMTK_API_SET_FIX_CTL_100_MILLIHERTZ);
      break;
    case UPDATE_200_MILLIHERTZ:
      sendCommand(PMTK_API_SET_FIX_CTL_200_MILLIHERTZ);
      break;
    default:
      TLogPlus::Log.warning("Unsupported GPS fix rate.");
      break;
  }
}

void GPSManager::setDataMode(GPSDataMode mode)
{
  switch(mode)
  {
    case RMC_ONLY:
      sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
      break;
    case GLL_ONLY:
      sendCommand(PMTK_SET_NMEA_OUTPUT_GLLONLY);
      break;
    case VTG_ONLY:
      sendCommand(PMTK_SET_NMEA_OUTPUT_VTGONLY);
      break;
    case GGA_ONLY:
      sendCommand(PMTK_SET_NMEA_OUTPUT_GGAONLY);
      break;
    case GSA_ONLY:
      sendCommand(PMTK_SET_NMEA_OUTPUT_GSAONLY);
      break;
    case GSV_ONLY:
      sendCommand(PMTK_SET_NMEA_OUTPUT_GSVONLY);
      break;
    case RMC_GGA:
      sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
      break;
    case RMC_GGA_GSA:
      sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGAGSA);
      break;
    case ALL_DATA:
      sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
      break;
    case NO_DATA:
      sendCommand(PMTK_SET_NMEA_OUTPUT_OFF);
      break;
    default:
      TLogPlus::Log.warningln("Unsupported data mode requested.");
      break;
  }
}

void GPSManager::setSerialBatchRead(bool readAllTogether) {
  _serialBatchRead = readAllTogether;
}

Adafruit_GPS& GPSManager::getGPS() { return _gps; }

uint32_t GPSManager::getLastDataReceivedTime() const { return _lastDataReceivedTimer; }

void GPSManager::printToLog() 
{
    TLogPlus::Log.infoln("GPS Baud: " + String(_baudRate));
    TLogPlus::Log.infoln("GPS Data:");
    TLogPlus::Log.infoln("Time: " + _timeStr);
    TLogPlus::Log.infoln("Date: " + _dateStr);
    TLogPlus::Log.infoln("Fix: " + _fixStr);
    TLogPlus::Log.infoln("Location: " + _locationStr);
    TLogPlus::Log.infoln("Speed: " + _speedStr);
    TLogPlus::Log.infoln("Angle: " + _angleStr);
    TLogPlus::Log.infoln("Altitude: " + _altitudeStr);
    TLogPlus::Log.infoln("Satellites: " + _satellitesStr);
    TLogPlus::Log.infoln("Antenna: " + _antennaStr);
}