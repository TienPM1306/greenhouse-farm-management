#ifndef MQTT_SERVER_H
#define MQTT_SERVER_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define MQTT_MAX_TOPIC_LENGTH   128
#define MQTT_MAX_PAYLOAD_LENGTH 512

class MQTTServer
{
public:
    static MQTTServer *getInstance();

    uint32_t initialize();
    uint32_t connect(const char *clientID);

    uint32_t publish(const char *topic, const char *payload);
    uint32_t subscribe(const char *topic);
    uint32_t loop();

    bool isConnected();

    const char *getReceivedTopic()   const;
    const char *getReceivedPayload() const;
    uint32_t    clearReceivedMessage();

    // Topic builders
    const char *buildSensorTopic();
    const char *buildServiceTopic();
    const char *buildResponseTopic();
    const char *buildOnlineTopic();
    const char *buildConfigTopic();
    const char *buildNewGatewayTopic();

    // Convenience publishers
    uint32_t publishSensor(const char *payload);
    uint32_t publishResponse(const char *payload);
    uint32_t publishOnline();
    uint32_t publishNewGateway(const uint8_t *mac, const char *ssid);

    // Subscriptions
    uint32_t subscribeService();
    uint32_t subscribeConfig();

    // Xóa trạng thái đăng ký Gateway (NVS) + ngừng nhận lệnh.
    uint32_t clearRegistration();

private:
    MQTTServer();
    ~MQTTServer() = default;
    MQTTServer(const MQTTServer &) = delete;
    MQTTServer &operator=(const MQTTServer &) = delete;

    static MQTTServer *mInstance;

    WiFiClientSecure mEspClient;
    PubSubClient     mClient;

    char mReceivedTopic[MQTT_MAX_TOPIC_LENGTH];
    char mReceivedPayload[MQTT_MAX_PAYLOAD_LENGTH];

    char mSensorTopic[MQTT_MAX_TOPIC_LENGTH];
    char mServiceTopic[MQTT_MAX_TOPIC_LENGTH];
    char mResponseTopic[MQTT_MAX_TOPIC_LENGTH];
    char mOnlineTopic[MQTT_MAX_TOPIC_LENGTH];
    char mConfigTopic[MQTT_MAX_TOPIC_LENGTH];
    char mNewGatewayTopic[MQTT_MAX_TOPIC_LENGTH];

    static constexpr const char *MQTT_BROKER      = "";
    static constexpr uint16_t    MQTT_PORT         = 8883;
    static constexpr const char *MQTT_USERNAME     = "";
    static constexpr const char *MQTT_PASSWORD     = "";
    static constexpr const char *MQTT_REG_PREFIX   = "";
    static constexpr const char *MQTT_REG_CODE     = "";

    static void callback(char *topic, byte *payload, unsigned int length);
    void handleMessage(char *topic, byte *payload, unsigned int length);
    void handleConfigMessage(const uint8_t *payload, unsigned int length);
};

#endif
