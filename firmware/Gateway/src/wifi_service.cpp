#include "wifi_service.h"
#include "common.h"

WiFiService *WiFiService::mInstance = nullptr;

WiFiService *WiFiService::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new WiFiService();
    }
    return mInstance;
}

WiFiService::WiFiService()
{
    mConnected = false;
}

uint32_t WiFiService::initialize()
{
    Serial.println(F("WiFi Setup..."));
    WiFi.mode(WIFI_STA);

    if (!mWifiManager.autoConnect("ESP32-Gateway")) {
        Serial.println(F("WiFi Connect Failed"));
        return RESULT_ERR;
    }

    Serial.println(F("WiFi Connected"));
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    mConnected = true;
    return RESULT_OK;
}

uint32_t WiFiService::connect()
{
    Serial.println(F("Connecting WiFi..."));
    WiFi.begin();

    for (uint8_t retry = 0; retry < 10 && WiFi.status() != WL_CONNECTED; retry++) {
        delay(1000);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        mConnected = true;
        return RESULT_OK;
    }

    mConnected = false;
    return RESULT_ERR;
}

uint32_t WiFiService::reconnect()
{
    if (WiFi.status() == WL_CONNECTED) {
        return RESULT_OK;
    }
    Serial.println(F("Reconnecting WiFi..."));
    return connect();
}

uint32_t WiFiService::reset()
{
    Serial.println(F("WiFi Reset..."));
    mWifiManager.resetSettings();
    delay(500);
    ESP.restart();
    return RESULT_OK;
}

bool WiFiService::connected()
{
    return WiFi.status() == WL_CONNECTED;
}

String WiFiService::getIP()
{
    return WiFi.localIP().toString();
}

String WiFiService::getSSID()
{
    return WiFi.SSID();
}
