#ifndef ESP_SERVICE_H
#define ESP_SERVICE_H

#include <Arduino.h>
#include <HardwareSerial.h>

// Serial communication interface
extern HardwareSerial Serial3;

/**
 * Initialize Serial communication for ESP interface
 * @param baudRate: Communication speed (typically 9600)
 */
void Serial_Setup(int baudRate);

/**
 * Process and print received data from ESP
 * @param data: Byte array received from serial
 * @param length: Length of the data
 */
void processerData(uint8_t *data, uint16_t length);

void processControlDevice(uint8_t* buffer, uint16_t size);

/**
 * Gửi kết quả relay (sau khi controlRelay) ngược về Node ESP32 qua Serial3.
 */
void sendControlResponse(const uint8_t* nodeMac, uint8_t deviceId);

#endif // ESP_SERVICE_H
