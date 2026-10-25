#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <WiFi.h>
#include <WiFiManager.h>

class WiFiService
{
public:
    static WiFiService *getInstance();

    uint32_t initialize();
    uint32_t connect();
    uint32_t reconnect();
    uint32_t reset();

    bool   connected();
    String getIP();
    String getSSID();

private:
    WiFiService();

    static WiFiService *mInstance;

    WiFiManager mWifiManager;
    bool        mConnected;
};

#endif
