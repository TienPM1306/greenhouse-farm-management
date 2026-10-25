#ifndef STM32_SERVICE_H
#define STM32_SERVICE_H

#include <Arduino.h>
#include <SoftwareSerial.h> // Thêm thư viện này

// Khớp dây thật trên PCB: Node GPIO26 (TX) -> NodeControl PB11 (RX),
// NodeControl PB10 (TX) -> Node GPIO35 (RX). GPIO35 là input-only trên ESP32
// nhưng SoftwareSerial chỉ cần đọc digital input ở chiều RX nên vẫn dùng được.
#define STM32_RX_PIN 35
#define STM32_TX_PIN 26

class STM32Service
{
public:
    static STM32Service *getInstance();
    uint32_t initialize();
    uint32_t sendCommand(const char *command);
    uint32_t receiveResponse(char *buffer, uint16_t maxSize);
    uint32_t sendData(uint8_t *data, uint16_t length);

    // Đọc phản hồi nhị phân từ STM32 (TLV, có thể chứa byte 0x00/0x0A nên
    // không dùng readBytesUntil). Gom byte tới khi gặp khoảng lặng inter-byte
    // (GAP_MS) hoặc hết timeoutMs. Trả RESULT_NO_DATA nếu không có gì đến.
    uint32_t receiveData(uint8_t *dataReceive, uint16_t *size, unsigned long timeoutMs);

private:
    STM32Service();
    static STM32Service *mInstance;
    SoftwareSerial *mSerial; // Đổi sang SoftwareSerial
};

#endif