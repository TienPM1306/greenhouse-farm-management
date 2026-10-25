#include "stm32_service.h"
#include "common.h"

STM32Service *STM32Service::mInstance = nullptr;

STM32Service *STM32Service::getInstance()
{
    if (mInstance == nullptr)
    {
        mInstance = new STM32Service();
    }
    return mInstance;
}

STM32Service::STM32Service()
{
    // Khởi tạo SoftwareSerial trên chân 25 (RX) và 26 (TX)
    mSerial = new SoftwareSerial(STM32_RX_PIN, STM32_TX_PIN);
}

uint32_t STM32Service::initialize()
{
    // SoftwareSerial thường hoạt động ổn định nhất ở tốc độ thấp (9600)
    mSerial->begin(9600);
    Serial.println(F("STM32Service (SoftwareSerial) initialized on Pins 25, 26"));
    return RESULT_OK;
}

uint32_t STM32Service::sendCommand(const char *command)
{
    mSerial->println(command);
    return RESULT_OK;
}

uint32_t STM32Service::receiveResponse(char *buffer, uint16_t maxSize)
{
    if (mSerial->available())
    {
        uint16_t len = mSerial->readBytesUntil('\n', buffer, maxSize - 1);
        buffer[len] = '\0';
        return RESULT_OK;
    }
    return RESULT_ERR;
}

uint32_t STM32Service::receiveData(uint8_t *dataReceive, uint16_t *size, unsigned long timeoutMs)
{
    uint16_t maxSize = *size;
    uint16_t length  = 0;

    // Chờ byte đầu tiên tới trong timeoutMs.
    unsigned long start = millis();
    while (!mSerial->available()) {
        if (millis() - start >= timeoutMs) {
            Serial.printf("[STM32] receiveData: khong co byte nao sau %lums\n", timeoutMs);
            *size = 0;
            return RESULT_NO_DATA;
        }
    }
    Serial.printf("[STM32] receiveData: byte dau tien sau %lums\n", millis() - start);

    // Gom byte tới khi gặp 20ms khoảng lặng (frame coi như đã đủ) — cùng quy
    // ước với NodeControl/src/main.cpp khi gom frame phía STM32.
    unsigned long lastByteMs = millis();
    while (millis() - lastByteMs < 20) {
        if (mSerial->available() && length < maxSize) {
            dataReceive[length++] = (uint8_t)mSerial->read();
            lastByteMs = millis();
        }
    }

    Serial.print(F("[STM32] receiveData raw: "));
    for (uint16_t i = 0; i < length; i++) {
        Serial.printf("%02X ", dataReceive[i]);
    }
    Serial.println();

    *size = length;
    return (length > 0) ? RESULT_OK : RESULT_NO_DATA;
}

uint32_t STM32Service::sendData(uint8_t *data, uint16_t length)
{
    if (data == nullptr || length == 0)
    {
        return RESULT_ERR;
    }

    // In debug ra cổng Serial chính của ESP32
    Serial.print("TX to STM32: ");
    for (uint16_t i = 0; i < length; i++)
    {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    // Gửi thực tế qua SoftwareSerial
    mSerial->write(data, length);
    return RESULT_OK;
}