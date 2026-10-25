#ifndef APP_ENGINE_H
#define APP_ENGINE_H

#include "common.h"
#include "lora_service.h"
#include "stm32_service.h"

class AppEngine {
public:
    static AppEngine* getInstance();

    uint32_t initialize();

    LoraService* getLoraService();

    STM32Service* getSTM32Service();

    void run();

private:
    AppEngine();
    ~AppEngine() = default;
    AppEngine(const AppEngine&) = delete;
    AppEngine& operator=(const AppEngine&) = delete;

    static AppEngine* mInstance;

    LoraService* mLoraService;

    STM32Service* mSTM32Service;

    unsigned long mLastReconnectCheck = 0;
    const unsigned long RECONNECT_INTERVAL = 5000;

    void handleLoraConnection();
};

#endif