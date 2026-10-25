#include "esp_service.h"
#include "device_control.h"
#include "command.h"
#include "message.h"
#include "protoc.h"

HardwareSerial Serial3(PB11, PB10);

/**
 * Initialize Serial communication for ESP interface
 * @param baudRate: Communication speed (typically 9600)
 */
void Serial_Setup(int baudRate)
{
    Serial3.begin(baudRate);
}

/**
 * Process and print received data from ESP
 * Simply outputs the received data without complex parsing
 * @param data: Byte array received from serial
 * @param length: Length of the data
 */
void processerData(uint8_t *data, uint16_t length)
{
    if(!data || !length) {
        return;
    }

    Serial1.print("RX: ");
    for (uint16_t i = 0; i < length; i++)
    {
        Serial1.print(data[i], HEX);
        Serial1.print(" ");
    }
    Serial1.println();
    
    uint8_t commandType = data[0];
    Serial1.print(F("Command Type: "));
    Serial1.println(commandType);
    
    // Process based on command type
    switch(commandType) {
        case CMD_CONTROL_DEVICE: {
            Serial1.println(F(">>> Processing CMD_CONTROL_DEVICE"));
            processControlDevice(data, length);
            break;
        }    
            
        default: {
            Serial1.print(F("Unknown command type: "));
            Serial1.println(commandType);
            break;
        }
    }
}

void processControlDevice(uint8_t* buffer, uint16_t size) {
    if (size < 1) {
        Serial1.println(F("Error: Packet too small"));
        return;
    }

    // Decode ControlDevice (skip command byte)
    ControlDevice controlDevice = {};
    ProtoCodec::decode(
        buffer + 1,
        size - 1,
        &controlDevice,
        ControlDevice_fields,
        ControlDevice_field_count
    );

    // Validate and print received data
    bool isValid = true;

    // Check if nodeMac exists
    if (!controlDevice.has_nodeMac) {
        Serial1.println(F("Error: nodeMac not found in packet"));
        isValid = false;
    } else {
        Serial1.print(F("Received NodeMac: "));
        for (int i = 0; i < 6; i++) {
            Serial1.print(controlDevice.nodeMac[i], HEX);
        }
        Serial1.println();
    }

    // Check if deviceId exists
    if (!controlDevice.has_deviceId) {
        Serial1.println(F("Error: deviceId not found in packet"));
        isValid = false;
    } else {
        Serial1.print(F("Device ID: "));
        Serial1.println(controlDevice.deviceId);
    }

    // Check if action exists
    if (!controlDevice.has_action) {
        Serial1.println(F("Error: action not found in packet"));
        isValid = false;
    } else {
        Serial1.print(F("Action: "));
        Serial1.println((uint8_t)controlDevice.action);
    }

    // If data is valid, send raw data to STM32 immediately
    if (isValid) {
        controlRelay(controlDevice.deviceId, controlDevice.action);
        sendControlResponse(controlDevice.nodeMac, controlDevice.deviceId);
    } else {
        Serial1.println(F("Invalid control device data. Data not sent to STM32."));
    }
}

/**
 * Gửi kết quả relay (sau khi controlRelay) ngược về Node ESP32 qua Serial3,
 * để Node forward CMD_CONTROL_RESPONSE về Gateway qua LoRa.
 */
void sendControlResponse(const uint8_t* nodeMac, uint8_t deviceId)
{
    ControlResponse resp = {};
    resp.has_nodeMac = true;
    memcpy(resp.nodeMac, nodeMac, 6);
    resp.has_deviceId = true;
    resp.deviceId = deviceId;
    resp.has_state = true;
    resp.state = (deviceId >= 0 && deviceId < NUM_RELAYS && relayStates[deviceId])
                     ? ControlDevice::CONTROL_ACTION_ON
                     : ControlDevice::CONTROL_ACTION_OFF;

    uint8_t buffer[32];
    buffer[0] = CMD_CONTROL_RESPONSE;
    uint16_t payloadSize = ProtoCodec::encode(
        buffer + 1, sizeof(buffer) - 1, &resp,
        ControlResponse_fields, ControlResponse_field_count);

    Serial3.write(buffer, 1 + payloadSize);

    Serial1.print(F("Sent ControlResponse to Node, deviceId="));
    Serial1.println(deviceId);
}

