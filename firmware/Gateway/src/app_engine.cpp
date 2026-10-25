#include "app_engine.h"
#include <Preferences.h>

AppEngine *AppEngine::mInstance = nullptr;

AppEngine *AppEngine::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new AppEngine();
    }
    return mInstance;
}

AppEngine::AppEngine()
{
    mWifiService = WiFiService::getInstance();
    mMqttServer  = MQTTServer::getInstance();
    mLoraService = LoraService::getInstance();
    mBuzzer      = BuzzerService::getInstance();
}

uint32_t AppEngine::initialize()
{
    Serial.println(F("=== AppEngine Initializing ==="));

    static LoraConfig loraConfig = {
        AIR_DATA_RATE_010_24,
        UART_BPS_9600,
        MODE_00_8N1,
        FT_TRANSPARENT_TRANSMISSION,
        FEC_1_ON,
        WAKE_UP_250,
        // Thí nghiệm 2026-07-02: hạ 17dBm -> 10dBm chống bão hòa đầu thu khi 2 board
        // đặt gần nhau (cùng phòng). Triển khai nhà kính thật (xa) thì trả lại POWER_17.
        POWER_10
    };

    if (mLoraService->initialize(&loraConfig) != RESULT_OK) {
        Serial.println(F("LoRa init failed"));
        mBuzzer->playLoRaError();
    } else {
        mPrevLoraConnected = true;
    }

    if (mWifiService->initialize() != RESULT_OK) {
        Serial.println(F("WiFi init failed"));
        return RESULT_ERR;
    }
    mPrevWifiConnected = true;
    mBuzzer->playWiFiSuccess();

    if (mMqttServer->initialize() != RESULT_OK) {
        Serial.println(F("MQTT init failed"));
        return RESULT_ERR;
    }

    Serial.println(F("=== AppEngine Ready ==="));
    return RESULT_OK;
}

void AppEngine::run()
{
    handleLoraConnection();
    handleWiFiConnection();
    handleMQTTConnection();
    maintainMQTTLoop();
    monitorConnectionEvents();
}

void AppEngine::handleLoraConnection()
{
    if (!mLoraService->connected()) {
        if (millis() - mLastReconnectCheck >= RECONNECT_INTERVAL) {
            mLastReconnectCheck = millis();
            Serial.println(F("LoRa disconnected. Reinitializing..."));

            static LoraConfig loraConfig = {
                AIR_DATA_RATE_010_24,
                UART_BPS_9600,
                MODE_00_8N1,
                FT_TRANSPARENT_TRANSMISSION,
                FEC_1_ON,
                WAKE_UP_250,
                POWER_10
            };
            mLoraService->initialize(&loraConfig);
        }
    }
}

void AppEngine::handleWiFiConnection()
{
    if (!mWifiService->connected()) {
        Serial.println(F("WiFi disconnected. Reconnecting..."));
        mWifiService->reconnect();
    }
}

void AppEngine::handleMQTTConnection()
{
    if (!mWifiService->connected()) return;

    if (!mMqttServer->isConnected()) {
        if (millis() - mLastReconnectCheck >= RECONNECT_INTERVAL) {
            mLastReconnectCheck = millis();
            Serial.println(F("MQTT disconnected. Reconnecting..."));
            if (mMqttServer->connect("ESP32_Gateway") == RESULT_OK) {
                if (isGatewayRegistered()) {
                    // Đã đăng ký → subscribe ngay
                    mMqttServer->subscribeService();
                    mMqttServer->subscribeConfig();
                    Serial.println(F("[ENGINE] Already registered → subscribed immediately"));
                } else {
                    // Chưa đăng ký → chỉ subscribe config, chờ server xác nhận
                    registerGateway();
                    Serial.println(F("[ENGINE] Waiting for server registration..."));
                }
                mBuzzer->playMQTTSuccess();
            }
        }
    }
}

void AppEngine::maintainMQTTLoop()
{
    mMqttServer->loop();
}

void AppEngine::monitorConnectionEvents()
{
    bool currWifi  = mWifiService->connected();
    bool currMqtt  = mMqttServer->isConnected();
    bool currLora  = mLoraService->connected();

    if (!mPrevWifiConnected && currWifi) {
        mBuzzer->playWiFiSuccess();
    } else if (mPrevWifiConnected && !currWifi) {
        mBuzzer->playWiFiDisconnect();
    }

    if (!mPrevMqttConnected && currMqtt) {
        mBuzzer->playMQTTSuccess();
    } else if (mPrevMqttConnected && !currMqtt) {
        mBuzzer->playMQTTDisconnect();
    }

    if (mPrevLoraConnected && !currLora) {
        mBuzzer->playLoRaError();
    }

    mPrevWifiConnected = currWifi;
    mPrevMqttConnected = currMqtt;
    mPrevLoraConnected = currLora;
}

bool AppEngine::isWifiConnected()   { return mWifiService->connected(); }
bool AppEngine::isMqttConnected()   { return mMqttServer->isConnected(); }

bool AppEngine::isGatewayRegistered()
{
    Preferences prefs;
    prefs.begin("gateway", true);
    bool reg = prefs.getBool("registered", false);
    prefs.end();
    return reg;
}

void AppEngine::registerGateway()
{
    mMqttServer->subscribeConfig();

    if (isGatewayRegistered()) {
        Serial.println(F("[ENGINE] Already registered, skipping newgateway"));
        return;
    }

    uint8_t mac[6];
    if (!getMacAddress(mac)) {
        Serial.println(F("[ENGINE] Cannot get MAC for registration"));
        return;
    }

    String ssid = WiFi.SSID();
    if (mMqttServer->publishNewGateway(mac, ssid.c_str()) == RESULT_OK) {
        Serial.printf("[ENGINE] Sent newgateway (SSID: %s)\n", ssid.c_str());
    } else {
        Serial.println(F("[ENGINE] publishNewGateway failed"));
    }
}

LoraService   *AppEngine::getLoraService()  { return mLoraService; }
MQTTServer    *AppEngine::getMqttServer()   { return mMqttServer;  }
WiFiService   *AppEngine::getWifiService()  { return mWifiService; }
BuzzerService *AppEngine::getBuzzer()       { return mBuzzer;      }
