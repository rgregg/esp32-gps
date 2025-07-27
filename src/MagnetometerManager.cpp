#include "MagnetometerManager.h"
#include <math.h>
#include <algorithm> // For std::min and std::max

MagnetometerManager::MagnetometerManager(AppSettings* appSettings) : accel(54321), mag(12345) {
    _isMoving = false;
    _lastMoveTime = 0;
    _lastAccelMagnitude = 0.0;
    _appSettings = appSettings;
    _magOffsetX = _appSettings->getFloat(SETTING_MAG_OFFSET_X, MAG_OFFSET_X_DEFAULT);
    _magOffsetY = _appSettings->getFloat(SETTING_MAG_OFFSET_Y, MAG_OFFSET_Y_DEFAULT);
    _magOffsetZ = _appSettings->getFloat(SETTING_MAG_OFFSET_Z, MAG_OFFSET_Z_DEFAULT);
    _isCalibrating = false;
    _minX = _minY = _minZ = 0.0f;
    _maxX = _maxY = _maxZ = 0.0f;
}

bool MagnetometerManager::begin() {
    if (!accel.begin() || !mag.begin()) {
        return false;
    }
    return true;
}

void MagnetometerManager::read() {
    accel.getEvent(&accel_event);
    mag.getEvent(&mag_event);

    float currentAccelMagnitude = sqrt(pow(accel_event.acceleration.x, 2) + pow(accel_event.acceleration.y, 2) + pow(accel_event.acceleration.z, 2));

    if (abs(currentAccelMagnitude - _lastAccelMagnitude) > MOTION_THRESHOLD) {
        if (!_isMoving) {
            _isMoving = true;
        }
        _lastMoveTime = millis();
    } else {
        if (_isMoving && (millis() - _lastMoveTime > MOTION_TIMEOUT)) {
            _isMoving = false;
        }
    }

    _lastAccelMagnitude = currentAccelMagnitude;

    if (_isCalibrating) {
        // Update min/max values for calibration
        _minX = std::min(_minX, mag_event.magnetic.x);
        _maxX = std::max(_maxX, mag_event.magnetic.x);
        _minY = std::min(_minY, mag_event.magnetic.y);
        _maxY = std::max(_maxY, mag_event.magnetic.y);
        _minZ = std::min(_minZ, mag_event.magnetic.z);
        _maxZ = std::max(_maxZ, mag_event.magnetic.z);
    }
}

float MagnetometerManager::getHeading() {
    // Calculate Roll and Pitch from accelerometer data
    float roll = atan2(accel_event.acceleration.y, accel_event.acceleration.z);
    float pitch = atan2(-accel_event.acceleration.x, sqrt(accel_event.acceleration.y * accel_event.acceleration.y + accel_event.acceleration.z * accel_event.acceleration.z));

    // Tilt compensation
    float Bx = mag_event.magnetic.x + _magOffsetX;
    float By = mag_event.magnetic.y + _magOffsetY;
    float Bz = mag_event.magnetic.z + _magOffsetZ;

    float Bx_comp = Bx * cos(pitch) + Bz * sin(pitch);
    float By_comp = Bx * sin(roll) * sin(pitch) + By * cos(roll) - Bz * sin(roll) * cos(pitch);

    // Calculate heading
    float heading = (atan2(By_comp, Bx_comp) * 180) / PI;

    if (heading < 0) {
        heading = 360 + heading;
    }
    return heading;
}

bool MagnetometerManager::isMoving() {
    return _isMoving;
}

void MagnetometerManager::setCalibrationOffsets(float offsetX, float offsetY, float offsetZ) {
    _magOffsetX = offsetX;
    _magOffsetY = offsetY;
    _magOffsetZ = offsetZ;
    _appSettings->setFloat(SETTING_MAG_OFFSET_X, _magOffsetX);
    _appSettings->setFloat(SETTING_MAG_OFFSET_Y, _magOffsetY);
    _appSettings->setFloat(SETTING_MAG_OFFSET_Z, _magOffsetZ);
}

void MagnetometerManager::getCalibrationOffsets(float& offsetX, float& offsetY, float& offsetZ) {
    offsetX = _magOffsetX;
    offsetY = _magOffsetY;
    offsetZ = _magOffsetZ;
}

bool MagnetometerManager::isCalibrationModeEnabled() {
    return _appSettings->getBool(SETTING_MAG_CALIBRATION_MODE_ENABLED, MAG_CALIBRATION_MODE_DEFAULT);
}

void MagnetometerManager::setCalibrationModeEnabled(bool enabled) {
    _appSettings->setBool(SETTING_MAG_CALIBRATION_MODE_ENABLED, enabled);
}

void MagnetometerManager::startCalibration() {
    _isCalibrating = true;
    _minX = _minY = _minZ = 0.0f;
    _maxX = _maxY = _maxZ = 0.0f;
}

void MagnetometerManager::stopCalibration() {
    _isCalibrating = false;

    // Calculate offsets using the min/max values
    _magOffsetX = -(_maxX + _minX) / 2.0f;
    _magOffsetY = -(_maxY + _minY) / 2.0f;
    _magOffsetZ = -(_maxZ + _minZ) / 2.0f;

    setCalibrationOffsets(_magOffsetX, _magOffsetY, _magOffsetZ);
}

void MagnetometerManager::getMinMaxX(float& min, float& max) {
    min = _minX;
    max = _maxX;
}

void MagnetometerManager::getMinMaxY(float& min, float& max) {
    min = _minY;
    max = _maxY;
}

void MagnetometerManager::getMinMaxZ(float& min, float& max) {
    min = _minZ;
    max = _maxZ;
}