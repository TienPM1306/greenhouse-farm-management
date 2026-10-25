#ifndef BUZZER_SERVICE_H
#define BUZZER_SERVICE_H

#include <Arduino.h>

#define BUZZER_PIN 14

class BuzzerService
{
public:
    static BuzzerService *getInstance();

    void initialize();

    void playWiFiConnecting();
    void playWiFiSuccess();
    void playWiFiDisconnect();

    void playMQTTSuccess();
    void playMQTTDisconnect();

    void playLoRaError();
    void playReset();
    void playTopicReceived();
    void playPingTest();
    void playRegistrationSuccess();

private:
    BuzzerService() = default;

    static BuzzerService *mInstance;
};

#endif
