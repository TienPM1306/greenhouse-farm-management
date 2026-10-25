#include <Arduino.h>
#include "app_engine.h"
#include "app_service.h"
#include "common.h"

void AppEngineTask(void *pvParameters) {
  (void)pvParameters;

  while(true){
    AppEngine::getInstance()->run();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void AppServiceTask(void *pvParameters) {
  (void)pvParameters;

  while(true){
    AppService::getInstance()->run();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void setup() 
{
    Serial.begin(115200);

    Serial.println(F("\n=== ESP32 MQTT System Starting ==="));

    AppEngine::getInstance()->initialize();
    AppService::getInstance()->initialize();

    xTaskCreate(AppEngineTask, "AppEngineTask", 16384, NULL, 1, NULL);
    xTaskCreate(AppServiceTask, "AppServiceTask", 16384, NULL, 1, NULL);
}

// ==================== LOOP ====================

void loop() {
    vTaskDelete(NULL);
}