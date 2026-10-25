#ifndef APP_ENGINE_H
#define APP_ENGINE_H

#include "common.h"
#include "mqtt.h"
#include "wifi_service.h"
#include "lora_service.h"
#include "buzzer_service.h"

class AppEngine
{
public:
    static AppEngine *getInstance();

    uint32_t initialize();
    void     run();

    bool isWifiConnected();
    bool isMqttConnected();
    bool isGatewayRegistered();

    LoraService  *getLoraService();
    MQTTServer   *getMqttServer();
    WiFiService  *getWifiService();
    BuzzerService *getBuzzer();

private:
    AppEngine();
    ~AppEngine() = default;
    AppEngine(const AppEngine &) = delete;
    AppEngine &operator=(const AppEngine &) = delete;

    static AppEngine *mInstance;

    WiFiService  *mWifiService;
    MQTTServer   *mMqttServer;
    LoraService  *mLoraService;
    BuzzerService *mBuzzer;

    unsigned long mLastReconnectCheck = 0;
    static constexpr unsigned long RECONNECT_INTERVAL = 5000;

    bool mPrevWifiConnected = false;
    bool mPrevMqttConnected = false;
    bool mPrevLoraConnected = false;

    void handleLoraConnection();
    void handleWiFiConnection();
    void handleMQTTConnection();
    void maintainMQTTLoop();
    void monitorConnectionEvents();
    void registerGateway();
};

#endif
