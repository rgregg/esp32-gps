#include "UDPManager.h"
#include <TLogPlus.h>

UDPManager::UDPManager(const char* destHost, uint16_t destPort)
    : _destPort(destPort), _hasBegun(false) {
    strncpy(_destHost, destHost, sizeof(_destHost) - 1);
    _destHost[sizeof(_destHost) - 1] = '\0';
    _listenPort = random(49152, 65535);
}

void UDPManager::begin() {
    if (WiFi.status() != WL_CONNECTED) {
        return; // Don't attempt to connect if WiFi is not available
    }

    TLogPlus::Log.infoln("UDP: Initializing UDP on port %u", _listenPort);
    if (_udp.begin(_listenPort)) {
        _hasBegun = true;
        TLogPlus::Log.infoln("UDP: Initalized.");
    } else {
        TLogPlus::Log.errorln("UDP: Failed to initalize.");
        _hasBegun = false;
    }
}

void UDPManager::stop() {
    _udp.stop();
    _hasBegun = false;
}

void UDPManager::send(const char* message) {
    if (!_hasBegun) {
        TLogPlus::Log.debugln("UDP tried to send data when not begun. May indicate a connection issue.");
        return;
    }

    _udp.beginPacket(_destHost, _destPort);
    _udp.write((const uint8_t*)message, strlen(message));
    int result = _udp.endPacket();
    if (result == 0) {
        TLogPlus::Log.debugln("UDP failed to send.");
    }
}

void UDPManager::setDestHost(const char* host) {
    strncpy(_destHost, host, sizeof(_destHost) - 1);
    _destHost[sizeof(_destHost) - 1] = '\0';
}

void UDPManager::setDestPort(uint16_t port) {
    _destPort = port;
}
