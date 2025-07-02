#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>

class UDPManager {
public:
    UDPManager(const char* destHost, uint16_t destPort);
    void begin();
    void send(const char* message);
    void setDestHost(const char* host);
    void setDestPort(uint16_t port);
    void stop();

private:
    char _destHost[64];
    uint16_t _listenPort;
    uint16_t _destPort;
    WiFiUDP _udp;
    bool _hasBegun;
};
