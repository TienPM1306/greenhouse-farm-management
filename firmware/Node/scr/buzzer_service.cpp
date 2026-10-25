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
    // Cấu hình LEDC một lần (không dùng tone()/noTone() để tránh lỗi LEDC + watchdog)
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, 10); // 10-bit resolution
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWrite(BUZZER_CHANNEL, 0); // tắt
    mIsOn = false;
}

void BuzzerService::alarmOn()
{
    if (mIsOn) {
        return; // đã bật rồi, không gọi lại mỗi vòng loop
    }
    ledcWriteTone(BUZZER_CHANNEL, BUZZER_FREQ); // phát tone liên tục
    mIsOn = true;
}

void BuzzerService::alarmOff()
{
    if (!mIsOn) {
        return; // đã tắt rồi
    }
    ledcWrite(BUZZER_CHANNEL, 0); // tắt
    mIsOn = false;
}
