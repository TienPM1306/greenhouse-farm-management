#include "command_handler.h"
#include "app_engine.h"
#include "app_service.h"
#include "mqtt.h"

// -----------------------------------------------------------------------
// Helper: validate LoRa availability and send a raw packet
// -----------------------------------------------------------------------
static bool loraForward(uint8_t cmdType, const uint8_t *data, unsigned int length)
{
    LoraService *lora = AppEngine::getInstance()->getLoraService();

    if (lora == nullptr) {
        Serial.println(F("[CMD] LoRa service unavailable"));
        return false;
    }
    if (!lora->connected()) {
        Serial.println(F("[CMD] LoRa not connected"));
        return false;
    }
    if (length > 255) {
        Serial.println(F("[CMD] Payload too large for LoRa"));
        return false;
    }

    uint8_t packet[1 + 256];
    packet[0] = cmdType;
    memcpy(packet + 1, data, length);

    uint32_t result = lora->send(packet, static_cast<uint16_t>(length + 1));
    if (result == RESULT_OK) {
        Serial.printf("[CMD] Forwarded cmd=0x%02X over LoRa (%u bytes)\n",
                      cmdType, (unsigned)(length + 1));
        return true;
    }
    Serial.printf("[CMD] LoRa send failed for cmd=0x%02X\n", cmdType);
    return false;
}

// -----------------------------------------------------------------------
// Main dispatcher
// -----------------------------------------------------------------------
void handleMqttCommand(uint8_t commandType, const uint8_t *data, unsigned int length)
{
    Serial.printf("[CMD] Received command type: 0x%02X  length: %u\n", commandType, length);

    switch (commandType) {

        // ---- CMD_CHANGE_GATEWAY_MAC ----
        case CMD_CHANGE_GATEWAY_MAC: {
            if (length == 0) { Serial.println(F("[CMD] ChangeGatewayMac: missing payload")); return; }

            ChangeGatewayMac msg = {};
            ProtoCodec::decode((uint8_t *)data, length, &msg,
                               ChangeGatewayMac_fields, ChangeGatewayMac_field_count);

            if (!msg.has_newGatewayMac || !msg.has_nodeMac) {
                Serial.println(F("[CMD] ChangeGatewayMac: incomplete fields"));
                return;
            }
            if (loraForward(commandType, data, length)) {
                AppService::getInstance()->registerPending(
                    commandType, data, length, msg.nodeMac, CMD_CHANGE_GATEWAY_RESPONSE);
            }
            break;
        }

        // ---- CMD_CONTROL_DEVICE ----
        case CMD_CONTROL_DEVICE: {
            if (length == 0) { Serial.println(F("[CMD] ControlDevice: missing payload")); return; }

            ControlDevice msg = {};
            ProtoCodec::decode((uint8_t *)data, length, &msg,
                               ControlDevice_fields, ControlDevice_field_count);

            if (!msg.has_nodeMac || !msg.has_deviceId || !msg.has_action) {
                Serial.println(F("[CMD] ControlDevice: incomplete fields"));
                return;
            }
            Serial.printf("[CMD] ControlDevice: deviceId=%d action=%d\n",
                          msg.deviceId, (uint8_t)msg.action);
            if (loraForward(commandType, data, length)) {
                AppService::getInstance()->registerControlCampaign(data, length, msg.nodeMac, msg.deviceId);
            }
            break;
        }

        // ---- CMD_STATE_DEVICE ----
        case CMD_STATE_DEVICE: {
            if (length == 0) { Serial.println(F("[CMD] StateDevice: missing payload")); return; }

            StateDevice msg = {};
            ProtoCodec::decode((uint8_t *)data, length, &msg,
                               StateDevice_fields, StateDevice_field_count);

            if (!msg.has_nodeMac || !msg.has_deviceId) {
                Serial.println(F("[CMD] StateDevice: incomplete fields"));
                return;
            }
            Serial.printf("[CMD] StateDevice: deviceId=%d\n", msg.deviceId);
            if (loraForward(commandType, data, length)) {
                AppService::getInstance()->registerPending(
                    commandType, data, length, msg.nodeMac, CMD_STATE_RESPONSE);
            }
            break;
        }

        // ---- CMD_SCAN_REQUEST ----
        // Khởi động chiến dịch quét 10s (mỗi 2s phát 1 gói broadcast) do
        // AppService thực hiện non-blocking; KHÔNG chặn callback MQTT.
        case CMD_SCAN_REQUEST: {
            Serial.println(F("[CMD] Scan request -> start 10s scan campaign"));
            AppService::getInstance()->startScan();
            break;
        }

        // ---- CMD_REQUEST_ALL_STATES ----
        case CMD_REQUEST_ALL_STATES: {
            if (length == 0) { Serial.println(F("[CMD] RequestAllStates: missing payload")); return; }

            RequestAllStates msg = {};
            ProtoCodec::decode((uint8_t *)data, length, &msg,
                               RequestAllStates_fields, RequestAllStates_field_count);

            if (!msg.has_nodeMac) {
                Serial.println(F("[CMD] RequestAllStates: missing nodeMac"));
                return;
            }
            if (loraForward(commandType, data, length)) {
                AppService::getInstance()->registerPending(
                    commandType, data, length, msg.nodeMac, CMD_ALL_STATES_RESPONSE);
            }
            break;
        }

        // ---- CMD_CHANGE_TIME ----
        case CMD_CHANGE_TIME: {
            if (length == 0) { Serial.println(F("[CMD] ChangeTime: missing payload")); return; }

            ChangeTime msg = {};
            ProtoCodec::decode((uint8_t *)data, length, &msg,
                               ChangeTime_fields, ChangeTime_field_count);

            if (!msg.has_nodeMac || !msg.has_newTime) {
                Serial.println(F("[CMD] ChangeTime: incomplete fields"));
                return;
            }
            Serial.printf("[CMD] ChangeTime: newTime=%u\n", msg.newTime);
            if (loraForward(commandType, data, length)) {
                AppService::getInstance()->registerPending(
                    commandType, data, length, msg.nodeMac, CMD_CHANGE_TIME_RESPONSE);
            }
            break;
        }

        // ---- CMD_DELETE_GATEWAY ----
        // Xóa Gateway: (1) xóa đăng ký của chính Gateway; (2) phát broadcast
        // 10s (5 gói) để các Node gắn Gateway này tự xóa GatewayMac đã lưu.
        case CMD_DELETE_GATEWAY: {
            Serial.println(F("[CMD] Delete gateway -> clear registration + notify nodes"));
            AppEngine::getInstance()->getMqttServer()->clearRegistration();
            AppService::getInstance()->startDeleteGateway();
            break;
        }

        // ---- CMD_WIFI_RESET (gateway-internal) ----
        case CMD_WIFI_RESET: {
            Serial.println(F("[CMD] WiFi Reset requested"));
            WiFiService *wifi = AppEngine::getInstance()->getWifiService();
            if (wifi != nullptr) {
                wifi->reset();
            }
            break;
        }

        default:
            Serial.printf("[CMD] Unhandled command: 0x%02X\n", commandType);
            break;
    }
}
