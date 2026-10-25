#ifndef BUZZER_SERVICE_H
#define BUZZER_SERVICE_H

#include <Arduino.h>

// TODO: đổi chân buzzer tại đây nếu cần (GPIO 14 đang trống trên Node)
#define BUZZER_PIN      14
// Kênh LEDC dùng để phát tần số cho buzzer (0-15, tránh trùng với module khác)
#define BUZZER_CHANNEL  0
// Tần số phát ra (Hz) khi cảnh báo
#define BUZZER_FREQ     2500

class BuzzerService
{
public:
    static BuzzerService *getInstance();

    void initialize();

    // Bật buzzer kêu liên tục khi thông số vượt ngưỡng
    void alarmOn();
    // Tắt buzzer khi thông số trở lại trong ngưỡng
    void alarmOff();

private:
    BuzzerService() = default;

    static BuzzerService *mInstance;
    bool mIsOn = false;
};

#endif
