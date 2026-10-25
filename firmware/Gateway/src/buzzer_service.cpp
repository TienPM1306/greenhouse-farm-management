#include "buzzer_service.h"

BuzzerService *BuzzerService::mInstance = nullptr;

BuzzerService *BuzzerService::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new BuzzerService();
    }
    return mInstance;
}

void BuzzerService::initialize()
{
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);
}

void BuzzerService::playWiFiConnecting()
{
    tone(BUZZER_PIN, 2000, 250);
    vTaskDelay(pdMS_TO_TICKS(350));
    noTone(BUZZER_PIN);
}

void BuzzerService::playWiFiSuccess()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    tone(BUZZER_PIN, 2000, 600);
    vTaskDelay(pdMS_TO_TICKS(500));
    tone(BUZZER_PIN, 2500, 200);
    vTaskDelay(pdMS_TO_TICKS(350));
    tone(BUZZER_PIN, 2500, 200);
    vTaskDelay(pdMS_TO_TICKS(350));
    noTone(BUZZER_PIN);
}

void BuzzerService::playWiFiDisconnect()
{
    tone(BUZZER_PIN, 2000, 400);
    vTaskDelay(pdMS_TO_TICKS(500));
    tone(BUZZER_PIN, 2000, 400);
    vTaskDelay(pdMS_TO_TICKS(500));
    noTone(BUZZER_PIN);
}

void BuzzerService::playMQTTSuccess()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    tone(BUZZER_PIN, 2000, 200);
    vTaskDelay(pdMS_TO_TICKS(250));
    tone(BUZZER_PIN, 2500, 200);
    vTaskDelay(pdMS_TO_TICKS(250));
    noTone(BUZZER_PIN);
}

void BuzzerService::playMQTTDisconnect()
{
    tone(BUZZER_PIN, 2000, 400);
    vTaskDelay(pdMS_TO_TICKS(500));
    tone(BUZZER_PIN, 2000, 400);
    vTaskDelay(pdMS_TO_TICKS(500));
    noTone(BUZZER_PIN);
}

void BuzzerService::playLoRaError()
{
    tone(BUZZER_PIN, 2500, 300);
    vTaskDelay(pdMS_TO_TICKS(320));
    tone(BUZZER_PIN, 2300, 300);
    vTaskDelay(pdMS_TO_TICKS(320));
    tone(BUZZER_PIN, 2100, 300);
    vTaskDelay(pdMS_TO_TICKS(320));
    tone(BUZZER_PIN, 2000, 300);
    vTaskDelay(pdMS_TO_TICKS(320));
    noTone(BUZZER_PIN);
}

void BuzzerService::playReset()
{
    tone(BUZZER_PIN, 2000, 700);
    vTaskDelay(pdMS_TO_TICKS(800));
    tone(BUZZER_PIN, 2000, 700);
    vTaskDelay(pdMS_TO_TICKS(800));
    tone(BUZZER_PIN, 2000, 700);
    vTaskDelay(pdMS_TO_TICKS(800));
    noTone(BUZZER_PIN);
}

void BuzzerService::playTopicReceived()
{
    tone(BUZZER_PIN, 2000, 120);
    vTaskDelay(pdMS_TO_TICKS(150));
    tone(BUZZER_PIN, 2000, 120);
    vTaskDelay(pdMS_TO_TICKS(150));
    noTone(BUZZER_PIN);
}

void BuzzerService::playPingTest()
{
    tone(BUZZER_PIN, 4000, 150);
    vTaskDelay(pdMS_TO_TICKS(220));
    tone(BUZZER_PIN, 4000, 150);
    vTaskDelay(pdMS_TO_TICKS(220));
    tone(BUZZER_PIN, 4000, 150);
    vTaskDelay(pdMS_TO_TICKS(220));
    noTone(BUZZER_PIN);
}

void BuzzerService::playRegistrationSuccess()
{
    tone(BUZZER_PIN, 1500, 150);
    vTaskDelay(pdMS_TO_TICKS(200));
    tone(BUZZER_PIN, 2000, 150);
    vTaskDelay(pdMS_TO_TICKS(200));
    tone(BUZZER_PIN, 2800, 150);
    vTaskDelay(pdMS_TO_TICKS(200));
    tone(BUZZER_PIN, 3800, 200);
    vTaskDelay(pdMS_TO_TICKS(300));
    noTone(BUZZER_PIN);
}
