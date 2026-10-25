#include "mqtt.h"
#include "common.h"
#include "common/command.h"
#include "common/message.h"
#include "common/protoc.h"
#include "command_handler.h"
#include "buzzer_service.h"
#include <Preferences.h>

// ---------- hex helpers ----------

static bool isHexDigit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static uint8_t hexValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return c - 'a' + 10;
}

static uint16_t parseHexPayload(const char *payload, unsigned int length,
                                uint8_t *output, uint16_t maxOutput)
{
    if (length & 1) return 0;
    uint16_t result = 0;
    for (unsigned int i = 0; i < length; i += 2) {
        if (result >= maxOutput) return 0;
        if (!isHexDigit(payload[i]) || !isHexDigit(payload[i + 1])) return 0;
        output[result++] = (hexValue(payload[i]) << 4) | hexValue(payload[i + 1]);
    }
    return result;
}

// ---------- Root CA ----------

static const char *mRootCa =
"-----BEGIN CERTIFICATE-----\n"
"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\n"
"-----END CERTIFICATE-----\n";

// ---------- Singleton ----------

MQTTServer *MQTTServer::mInstance = nullptr;

MQTTServer *MQTTServer::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new MQTTServer();
    }
    return mInstance;
}

MQTTServer::MQTTServer() : mClient(mEspClient)
{
    mReceivedTopic[0]    = '\0';
    mReceivedPayload[0]  = '\0';
    mSensorTopic[0]      = '\0';
    mServiceTopic[0]     = '\0';
    mResponseTopic[0]    = '\0';
    mOnlineTopic[0]      = '\0';
    mConfigTopic[0]      = '\0';
    mNewGatewayTopic[0]  = '\0';
}

// ---------- Init / Connect ----------

uint32_t MQTTServer::initialize()
{
    mEspClient.setCACert(mRootCa);
    mClient.setServer(MQTT_BROKER, MQTT_PORT);
    mClient.setCallback(callback);
    mClient.setBufferSize(MQTT_MAX_PAYLOAD_LENGTH);

    buildSensorTopic();
    buildServiceTopic();
    buildResponseTopic();
    buildOnlineTopic();
    buildConfigTopic();
    buildNewGatewayTopic();

    return RESULT_OK;
}

uint32_t MQTTServer::connect(const char *clientID)
{
    Serial.println(F("Connecting to MQTT broker..."));
    if (mClient.connect(clientID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println(F("MQTT Connected"));
        return RESULT_OK;
    }
    Serial.printf("MQTT failed, rc=%d\n", mClient.state());
    return RESULT_ERR;
}

// ---------- Pub / Sub ----------

uint32_t MQTTServer::publish(const char *topic, const char *payload)
{
    if (!mClient.connected()) return RESULT_ERR;
    return mClient.publish(topic, payload) ? RESULT_OK : RESULT_ERR;
}

uint32_t MQTTServer::subscribe(const char *topic)
{
    if (!mClient.connected()) return RESULT_ERR;
    if (mClient.subscribe(topic)) {
        Serial.printf("Subscribed: %s\n", topic);
        return RESULT_OK;
    }
    Serial.printf("Subscribe failed: %s\n", topic);
    return RESULT_ERR;
}

uint32_t MQTTServer::loop()
{
    mClient.loop();
    return RESULT_OK;
}

bool MQTTServer::isConnected()
{
    return mClient.connected();
}

// ---------- Received message ----------

const char *MQTTServer::getReceivedTopic()   const { return mReceivedTopic;   }
const char *MQTTServer::getReceivedPayload() const { return mReceivedPayload; }

uint32_t MQTTServer::clearReceivedMessage()
{
    mReceivedTopic[0]   = '\0';
    mReceivedPayload[0] = '\0';
    return RESULT_OK;
}

// ---------- Callbacks ----------

void MQTTServer::callback(char *topic, byte *payload, unsigned int length)
{
    MQTTServer::getInstance()->handleMessage(topic, payload, length);
}

void MQTTServer::handleMessage(char *topic, byte *payload, unsigned int length)
{
    strncpy(mReceivedTopic, topic, MQTT_MAX_TOPIC_LENGTH - 1);
    mReceivedTopic[MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

    if (length >= MQTT_MAX_PAYLOAD_LENGTH)
        length = MQTT_MAX_PAYLOAD_LENGTH - 1;
    memcpy(mReceivedPayload, payload, length);
    mReceivedPayload[length] = '\0';

    Serial.printf("[MQTT] Topic: %s  len: %u\n", mReceivedTopic, length);

    if (length == 0) return;

    // Config topic carries raw binary Protobuf — handle before hex-decode path
    if (mConfigTopic[0] != '\0' && strcmp(topic, mConfigTopic) == 0) {
        handleConfigMessage(payload, length);
        return;
    }

    uint8_t  binaryPayload[MQTT_MAX_PAYLOAD_LENGTH / 2] = {0};
    uint16_t binaryLength  = 0;
    bool     parsedAsHex   = false;

    if ((length & 1) == 0) {
        bool allHex = true;
        for (unsigned int i = 0; i < length && allHex; i++) {
            if (!isHexDigit(mReceivedPayload[i])) allHex = false;
        }
        if (allHex) {
            binaryLength = parseHexPayload(mReceivedPayload, length,
                                           binaryPayload, sizeof(binaryPayload));
            parsedAsHex = (binaryLength > 0);
        }
    }

    if (parsedAsHex) {
        handleMqttCommand(binaryPayload[0], binaryPayload + 1, binaryLength - 1);
    } else {
        handleMqttCommand(payload[0], payload + 1, length - 1);
    }
}

void MQTTServer::handleConfigMessage(const uint8_t *payload, unsigned int length)
{
    // Auto-detect hex string payload và decode về binary
    // Trim trailing whitespace/newline trước khi kiểm tra
    unsigned int trimLen = length;
    while (trimLen > 0 && (payload[trimLen - 1] == '\n' ||
                            payload[trimLen - 1] == '\r' ||
                            payload[trimLen - 1] == ' ')) {
        trimLen--;
    }

    uint8_t  binary[32] = {};
    uint16_t binaryLen  = 0;
    bool     isHex      = false;

    if ((trimLen & 1) == 0 && trimLen > 0 && trimLen <= sizeof(binary) * 2) {
        isHex = true;
        for (unsigned int i = 0; i < trimLen && isHex; i++) {
            if (!isHexDigit((char)payload[i])) isHex = false;
        }
        if (isHex) {
            binaryLen = parseHexPayload((const char *)payload, trimLen, binary, sizeof(binary));
            isHex = (binaryLen > 0);
        }
    }

    const uint8_t *decodePtr = isHex ? binary      : payload;
    uint16_t       decodeLen = isHex ? binaryLen   : (uint16_t)length;

    Serial.printf("[CONFIG] raw(%u): ", decodeLen);
    for (uint16_t i = 0; i < decodeLen; i++) Serial.printf("%02X ", decodePtr[i]);
    Serial.println();

    ConfigMsg msg = {};
    ProtoCodec::decode((uint8_t *)decodePtr, decodeLen, &msg,
                       ConfigMsg_fields, ConfigMsg_field_count);

    Serial.printf("[CONFIG] has_pingTest=%d pingTest=%d | has_isConnected=%d isConnected=%d\n",
                  msg.has_pingTest, msg.pingTest,
                  msg.has_isConnectedServer, msg.isConnectedServer);

    if (msg.has_pingTest) {
        if (msg.pingTest) {
            // pingTest = true: server ping để xác nhận đúng gateway
            Serial.println(F("[MQTT] Ping test received → buzzing"));
            BuzzerService::getInstance()->playPingTest();
        } else {
            // pingTest = false: server yêu cầu unregister gateway
            Serial.println(F("[MQTT] Unregistration received → clearing NVS"));
            Preferences prefs;
            prefs.begin("gateway", false);
            prefs.putBool("registered", false);
            prefs.end();
            mClient.unsubscribe(mServiceTopic);
            Serial.println(F("[MQTT] Unregistered → unsubscribed from commands"));
        }
        return;
    }

    if (!msg.has_isConnectedServer) {
        Serial.println(F("[MQTT] ConfigMsg: no isConnectedServer field"));
        return;
    }

    Serial.printf("[MQTT] Config: isConnectedServer=%d\n", msg.isConnectedServer);

    if (msg.isConnectedServer) {
        Preferences prefs;
        prefs.begin("gateway", false);
        prefs.putBool("registered", true);
        prefs.end();
        Serial.println(F("[MQTT] Registered → saved to NVS"));
        subscribeService();
        Serial.println(F("[MQTT] Registration complete → subscribed to commands"));
        BuzzerService::getInstance()->playRegistrationSuccess();
    } else {
        Preferences prefs;
        prefs.begin("gateway", false);
        prefs.putBool("registered", false);
        prefs.end();
        Serial.println(F("[MQTT] Reset registration → cleared NVS, restarting..."));
        mClient.disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP.restart();
    }
}

// ---------- Topic builders ----------

static bool buildMacStr(char *buffer, size_t len)
{
    uint8_t mac[6];
    if (!getMacAddress(mac)) { buffer[0] = '\0'; return false; }
    snprintf(buffer, len, "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return true;
}

const char *MQTTServer::buildSensorTopic()
{
    char macStr[13];
    if (!buildMacStr(macStr, sizeof(macStr))) { mSensorTopic[0] = '\0'; return mSensorTopic; }
    snprintf(mSensorTopic, MQTT_MAX_TOPIC_LENGTH, "%s/node_data", macStr);
    return mSensorTopic;
}

const char *MQTTServer::buildServiceTopic()
{
    char macStr[13];
    if (!buildMacStr(macStr, sizeof(macStr))) { mServiceTopic[0] = '\0'; return mServiceTopic; }
    snprintf(mServiceTopic, MQTT_MAX_TOPIC_LENGTH, "%s/commands", macStr);
    return mServiceTopic;
}

const char *MQTTServer::buildResponseTopic()
{
    char macStr[13];
    if (!buildMacStr(macStr, sizeof(macStr))) { mResponseTopic[0] = '\0'; return mResponseTopic; }
    snprintf(mResponseTopic, MQTT_MAX_TOPIC_LENGTH, "%s/node_response", macStr);
    return mResponseTopic;
}

const char *MQTTServer::buildOnlineTopic()
{
    char macStr[13];
    if (!buildMacStr(macStr, sizeof(macStr))) { mOnlineTopic[0] = '\0'; return mOnlineTopic; }
    snprintf(mOnlineTopic, MQTT_MAX_TOPIC_LENGTH, "%s/online", macStr);
    return mOnlineTopic;
}

const char *MQTTServer::buildConfigTopic()
{
    char macStr[13];
    if (!buildMacStr(macStr, sizeof(macStr))) { mConfigTopic[0] = '\0'; return mConfigTopic; }
    snprintf(mConfigTopic, MQTT_MAX_TOPIC_LENGTH, "%s/config", macStr);
    return mConfigTopic;
}

const char *MQTTServer::buildNewGatewayTopic()
{
    snprintf(mNewGatewayTopic, MQTT_MAX_TOPIC_LENGTH, "%s/%s/newgateway",
             MQTT_REG_PREFIX, MQTT_REG_CODE);
    return mNewGatewayTopic;
}

// ---------- Convenience publishers ----------

uint32_t MQTTServer::publishSensor(const char *payload)
{
    if (mSensorTopic[0] == '\0') return RESULT_ERR;
    return publish(mSensorTopic, payload);
}

uint32_t MQTTServer::publishResponse(const char *payload)
{
    if (mResponseTopic[0] == '\0') return RESULT_ERR;
    return publish(mResponseTopic, payload);
}

uint32_t MQTTServer::publishOnline()
{
    if (mOnlineTopic[0] == '\0') return RESULT_ERR;
    return publish(mOnlineTopic, "1");
}

uint32_t MQTTServer::subscribeService()
{
    if (mServiceTopic[0] == '\0') buildServiceTopic();
    return subscribe(mServiceTopic);
}

uint32_t MQTTServer::clearRegistration()
{
    Preferences prefs;
    prefs.begin("gateway", false);
    prefs.putBool("registered", false);
    prefs.end();

    if (mClient.connected() && mServiceTopic[0] != '\0') {
        mClient.unsubscribe(mServiceTopic);
    }

    Serial.println(F("[MQTT] Gateway deleted -> registration cleared, unsubscribed"));
    return RESULT_OK;
}

uint32_t MQTTServer::subscribeConfig()
{
    if (mConfigTopic[0] == '\0') buildConfigTopic();
    return subscribe(mConfigTopic);
}

uint32_t MQTTServer::publishNewGateway(const uint8_t *mac, const char *ssid)
{
    if (mNewGatewayTopic[0] == '\0') return RESULT_ERR;
    if (!mClient.connected()) return RESULT_ERR;

    // field 1: macAddress  [01][01][6 bytes]
    // field 2: wifiSsid    [02][01][32 bytes ASCII, zero-padded]
    uint8_t  buf[2 + 6 + 2 + 32] = {};
    uint16_t idx = 0;

    buf[idx++] = 1;
    buf[idx++] = PROTO_UINT8;
    memcpy(buf + idx, mac, 6);
    idx += 6;

    buf[idx++] = 2;
    buf[idx++] = PROTO_UINT8;
    size_t ssidLen = strlen(ssid);
    if (ssidLen > 32) ssidLen = 32;
    memcpy(buf + idx, ssid, ssidLen);
    idx += 32;

    Serial.printf("[MQTT] publishNewGateway → %s (%u bytes)\n", mNewGatewayTopic, idx);
    Serial.print(F("[MQTT] buf: "));
    for (uint16_t i = 0; i < idx; i++) {
        Serial.printf("%02X ", buf[i]);
    }
    Serial.println();
    return mClient.publish(mNewGatewayTopic, buf, idx) ? RESULT_OK : RESULT_ERR;
}

