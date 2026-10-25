#include "app_engine.h"

AppEngine* AppEngine::mInstance = nullptr;

AppEngine* AppEngine::getInstance() {
    if (mInstance == nullptr) {
        mInstance = new AppEngine();
    }
    return mInstance;
}

AppEngine::AppEngine() {
    mLoraService = LoraService::getInstance();
    mSTM32Service = STM32Service::getInstance();
}

uint32_t AppEngine::initialize() {
    Serial.println(F("=== AppEngine Initializing ==="));

    // Lora Config
    static LoraConfig loraConfig =
    {
        AIR_DATA_RATE_010_24,
        UART_BPS_9600,
        MODE_00_8N1,
        FT_TRANSPARENT_TRANSMISSION,
        FEC_1_ON,
        WAKE_UP_250,
        POWER_10
    };

    if (mLoraService->initialize(&loraConfig) != RESULT_OK) {
        Serial.println(F("Lora initialization failed!"));
        return RESULT_ERR;
    }

    if (mSTM32Service->initialize() != RESULT_OK) {
        Serial.println(F("STM32 initialization failed!"));
        return RESULT_ERR;
    }

    Serial.println(F("AppEngine initialized successfully"));
    return RESULT_OK;
}

LoraService* AppEngine::getLoraService() {
    return mLoraService;
}

STM32Service* AppEngine::getSTM32Service() {
    return mSTM32Service;
}

void AppEngine::run() {
    handleLoraConnection();
}


void AppEngine::handleLoraConnection() {

    if (!mLoraService->isConnected()) {

        if (millis() - mLastReconnectCheck >= RECONNECT_INTERVAL) {

            mLastReconnectCheck = millis();

            Serial.println(F("Lora disconnected. Reconnecting..."));

            // re-init lora
            static LoraConfig loraConfig =
            {
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