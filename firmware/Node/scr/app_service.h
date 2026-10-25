#ifndef APP_SERVICE_H
#define APP_SERVICE_H

#include "common.h"
#include "message.h"
#include "command.h"
#include "protoc.h"
#include "dht_sensor.h"
#include "light_sensor.h"
#include "gateway_mac_manager.h"
#include "buzzer_service.h"
#include "stm32_service.h"

// ==================== NGƯỠNG CẢNH BÁO ====================
// TODO: chỉnh các ngưỡng cảnh báo bên dưới cho phù hợp thực tế
// Cảnh báo khi thông số > ngưỡng trên (HIGH) HOẶC < ngưỡng dưới (LOW)
#define TEMP_THRESHOLD_HIGH      35.0f   // Nhiệt độ (°C) vượt mức này -> cảnh báo (khớp quạt auto FAN_HOT + cảnh báo web)
#define TEMP_THRESHOLD_LOW       10.0f   // Nhiệt độ (°C) thấp hơn mức này -> cảnh báo

#define HUMIDITY_THRESHOLD_HIGH  90.0f   // Độ ẩm (%) vượt mức này -> cảnh báo
#define HUMIDITY_THRESHOLD_LOW   30.0f   // Độ ẩm (%) thấp hơn mức này -> cảnh báo

// Ánh sáng KHÔNG dùng cho cảnh báo buzzer (cảm biến không báo lỗi -> kêu nhầm liên tục).
// Giữ define để tham khảo nếu sau này muốn bật lại, nhưng checkSensorThresholds() không dùng.
// #define LIGHT_THRESHOLD_HIGH     3000.0f
// #define LIGHT_THRESHOLD_LOW      20.0f

class AppService {
public:
    static AppService* getInstance();

    uint32_t initialize();
    void run();
    
private:
    AppService();
    ~AppService() = default;
    AppService(const AppService&) = delete;
    AppService& operator=(const AppService&) = delete;

    static AppService* mInstance;

    DHTSensor* mDHTSensor = nullptr;
    LightSensor* mLightSensor = nullptr;
    BuzzerService* mBuzzerService = nullptr;
    SensorData mSensorData = {};
    uint8_t mDeviceNodeMac[6] = {};
    int mLastScanSession = -1;   // sessionId đợt quét gần nhất đã phản hồi (-1: chưa)

    uint32_t initializeSensors();
    void sendSensorData();
    void readSensor();
    void checkSensorThresholds();
    void receiveAndProcessLoraData();
    void processChangeGatewayMacPacket(uint8_t* buffer, uint16_t size);
    void sendChangeGatewayResponse(const uint8_t* nodeMac, bool success);
    void processControlDevice(uint8_t* buffer, uint16_t size);
    void processScanRequest(uint8_t* buffer, uint16_t size);
    void processDeleteGateway(uint8_t* buffer, uint16_t size);
    void printRawData(const char* label, uint8_t* data, uint16_t size);
    bool isNodeMacMatch(const uint8_t* receivedMac);

    // ACK "đã nhận lệnh": tự phát CMD_CONTROL_RESPONSE về Gateway ngay khi nhận
    // lệnh hợp lệ (state = action yêu cầu), không chờ phản hồi relay từ STM32
    // (spec 2026-07-02-node-receipt-ack-design.md mục 3.1).
    void sendReceiptAck(const ControlDevice& cmd);
};

#endif 