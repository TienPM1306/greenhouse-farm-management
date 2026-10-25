#include <Arduino.h>
#include "common.h"
#include "app_engine.h"
#include "app_service.h"
#include "buzzer_service.h"

// -----------------------------------------------------------------------
// FreeRTOS Tasks
// -----------------------------------------------------------------------

void AppEngineTask(void *pvParameters)
{
    (void)pvParameters;
    while (true) {
        AppEngine::getInstance()->run();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void AppServiceTask(void *pvParameters)
{
    (void)pvParameters;
    while (true) {
        AppService::getInstance()->run();
        // run() already calls vTaskDelay internally
    }
}

// -----------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n=== ESP32 Gateway Starting ==="));

    // Print gateway MAC
    uint8_t mac[6];
    if (getMacAddress(mac)) {
        char macStr[13];
        formatMacAddress(mac, macStr, sizeof(macStr));
        Serial.printf("Gateway MAC: %s\n", macStr);
    }

    // Init buzzer first for user feedback during boot
    BuzzerService::getInstance()->initialize();

    // Initialize Engine (LoRa + WiFi + MQTT)
    AppEngine::getInstance()->initialize();

    // Initialize Service layer
    AppService::getInstance()->initialize();

    // Start FreeRTOS tasks
    xTaskCreate(AppEngineTask,  "EngineTask",  8192, nullptr, 2, nullptr);
    xTaskCreate(AppServiceTask, "ServiceTask", 8192, nullptr, 1, nullptr);

    Serial.println(F("=== Tasks started ==="));
}

// -----------------------------------------------------------------------
// Loop — all work is done in FreeRTOS tasks
// -----------------------------------------------------------------------

void loop()
{
    vTaskDelete(nullptr);
}
