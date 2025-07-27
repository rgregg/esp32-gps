#ifndef MAGNETOMETER_MANAGER_H
#define MAGNETOMETER_MANAGER_H

#include <Adafruit_LSM303_Accel.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_Sensor.h>
#include "AppSettings.h"
#include <vector>

// Threshold for motion detection (in m/s^2)
#define MOTION_THRESHOLD 0.5
// Timeout for motion stop detection (in milliseconds)
#define MOTION_TIMEOUT 2000

class MagnetometerManager {
public:
    MagnetometerManager(AppSettings* appSettings);
    bool begin();
    void read();
    float getHeading();
    bool isMoving();
    void setCalibrationOffsets(float offsetX, float offsetY, float offsetZ);
    void getCalibrationOffsets(float& offsetX, float& offsetY, float& offsetZ);
    bool isCalibrationModeEnabled();
    void setCalibrationModeEnabled(bool enabled);
    void startCalibration();
    void stopCalibration();
    void getMinMaxX(float& min, float& max);
    void getMinMaxY(float& min, float& max);
    void getMinMaxZ(float& min, float& max);

private:
    Adafruit_LSM303_Accel_Unified accel;
    Adafruit_LIS2MDL mag;
    sensors_event_t accel_event;
    sensors_event_t mag_event;
    bool _isMoving;
    unsigned long _lastMoveTime;
    float _lastAccelMagnitude;
    float _magOffsetX, _magOffsetY, _magOffsetZ;
    AppSettings* _appSettings;
    bool _isCalibrating;
    std::vector<float> _magXData;
    std::vector<float> _magYData;
    std::vector<float> _magZData;
    float _minX, _maxX, _minY, _maxY, _minZ, _maxZ;
};

#endif // MAGNETOMETER_MANAGER_H
