#include "app_service.h"
#include "app_engine.h"

AppService* AppService::mInstance = nullptr;

AppService* AppService::getInstance() {
    if (mInstance == nullptr) {
        mInstance = new AppService();
    }
    return mInstance;
}

AppService::AppService() {}

uint32_t AppService::initialize() {
    Serial.println(F("AppService initialized"));
    
    // Get device's own MAC address
    if (getMacAddress(mDeviceNodeMac)) {
        Serial.print(F("Device NodeMac: "));
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", mDeviceNodeMac[i]);
        }
        Serial.println();
    } else {
        Serial.println(F("Warning: Failed to get device MAC address"));
    }
    
    // Initialize GatewayMacManager
    GatewayMacManager *gwMac = GatewayMacManager::getInstance();
    uint8_t defaultMac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    gwMac->initialize(defaultMac);
    
    initializeSensors();

    // Khởi tạo buzzer cảnh báo
    mBuzzerService = BuzzerService::getInstance();
    mBuzzerService->initialize();

    return RESULT_OK;
}

uint32_t AppService::initializeSensors() {
    mDHTSensor = new DHTSensor(27);
    if (mDHTSensor->initialize() != RESULT_OK) {
        Serial.println(F("DHT Sensor init failed"));
        return RESULT_ERR;
    }

    mLightSensor = new LightSensor(32); // GPIO 32 for Light
    if (mLightSensor->initialize() != RESULT_OK) {
        Serial.println(F("Light Sensor init failed"));
        return RESULT_ERR;
    }

    return RESULT_OK;
}

void AppService::run() {
    readSensor();
    checkSensorThresholds();
    sendSensorData();
    receiveAndProcessLoraData();
}

void AppService::sendSensorData() {
    static unsigned long lastSend = 0;
    const unsigned long SEND_INTERVAL = 5000;

    AppEngine* engine = AppEngine::getInstance();

    if (!engine->getLoraService()->isConnected()) {
        return;
    }

    if (millis() - lastSend < SEND_INTERVAL) {
        return;
    }

    // Fill in node MAC address from device
    uint8_t nodeMac[6] = {0};
    if (!getMacAddress(nodeMac)) {
        Serial.println(F("Failed to get node MAC address"));
        return;
    }
    memcpy(mSensorData.identity.nodeMac, nodeMac, 6);
    mSensorData.identity.has_nodeMac = true;

    // Get gateway MAC from NVS using GatewayMacManager
    uint8_t gatewayMac[6] = {};
    GatewayMacManager *gwMac = GatewayMacManager::getInstance();
    if (!gwMac->getCurrentMac(gatewayMac)) {
        Serial.println(F("Failed to get gateway MAC from NVS"));
        return;
    }
    memcpy(mSensorData.identity.gatewayMac, gatewayMac, 6);
    mSensorData.identity.has_gatewayMac = true;

    // Create buffer with command byte + encoded data
    uint8_t buffer[65]; // Command byte + SensorData
    buffer[0] = CMD_SYNC_SENSOR_DATA;

    uint16_t payloadSize = ProtoCodec::encode(
        buffer + 1,  // Encode after command byte
        sizeof(buffer) - 1,
        &mSensorData,
        SensorData_fields,
        SensorData_field_count);

    if (payloadSize == 0) {
        return;
    }

    // Total size is command byte + encoded data
    uint16_t totalSize = 1 + payloadSize;

    // In ra dữ liệu cảm biến được gửi
    auto* data = &mSensorData.data;
    Serial.println(F("Sending sensor data:"));
    if (data->has_light) {
        Serial.print(F("Light: "));
        Serial.println(data->light);
    }
    if (data->has_temperature) {
        Serial.print(F("Temperature: "));
        Serial.println(data->temperature);
    }
    if (data->has_humidity) {
        Serial.print(F("Humidity: "));
        Serial.println(data->humidity);
    }

    // In ra raw data đã encode (including command byte)
    Serial.print(F("Encoded data with command ("));
    Serial.print(totalSize);
    Serial.print(F(" bytes): "));
    for (uint16_t i = 0; i < totalSize; i++) {
        Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();

    lastSend = millis();

    LoraService* lora = engine->getLoraService();
    uint32_t result = lora->send(buffer, totalSize);

    if (result != RESULT_OK) {
        Serial.println(F("AppService: Send sensor data failed"));
    }
}

void AppService::readSensor() {
    static unsigned long lastSensorRead = 0;
    const unsigned long SENSOR_READ_INTERVAL = 2000;

    if (millis() - lastSensorRead < SENSOR_READ_INTERVAL) {
        return;
    }

    lastSensorRead = millis();

    mSensorData.data.has_light = false;
    mSensorData.data.has_temperature = false;
    mSensorData.data.has_humidity = false;

    bool readOk = false;

    if (mDHTSensor) {
        if (mDHTSensor->getData(mSensorData.data) == RESULT_OK) {
            readOk = true;
        }
    }

    if (mLightSensor) {
        if (mLightSensor->getData(mSensorData.data) == RESULT_OK) {
            readOk = true;
        }
    }

    if (!readOk) {
        Serial.println("Read sensor failed");
    }
}

void AppService::checkSensorThresholds() {
    if (mBuzzerService == nullptr) {
        return;
    }

    auto* data = &mSensorData.data;
    bool exceeded = false;

    // Cảnh báo khi vượt ngưỡng trên HOẶC dưới ngưỡng dưới
    if (data->has_temperature &&
        (data->temperature > TEMP_THRESHOLD_HIGH || data->temperature < TEMP_THRESHOLD_LOW)) {
        Serial.print(F("[ALERT] Temperature ngoai nguong: "));
        Serial.println(data->temperature);
        exceeded = true;
    }
    if (data->has_humidity &&
        (data->humidity > HUMIDITY_THRESHOLD_HIGH || data->humidity < HUMIDITY_THRESHOLD_LOW)) {
        Serial.print(F("[ALERT] Humidity ngoai nguong: "));
        Serial.println(data->humidity);
        exceeded = true;
    }
    // Ánh sáng KHÔNG dùng cho cảnh báo buzzer: cảm biến ánh sáng không có cơ chế báo lỗi
    // (LightSensor::checkData luôn return true) nên giá trị rác/dây hở dễ rơi ngoài ngưỡng
    // → buzzer kêu liên tục. Vẫn đọc & gửi light lên server, chỉ tách khỏi logic báo động.

    if (exceeded) {
        mBuzzerService->alarmOn();   // vượt ngưỡng -> kêu liên tục
    } else {
        mBuzzerService->alarmOff();  // trong ngưỡng -> tắt
    }
}

void AppService::receiveAndProcessLoraData() {
    AppEngine* engine = AppEngine::getInstance();
    LoraService* lora = engine->getLoraService();
    
    if (!lora->isConnected()) {
        return;
    }
    
    uint8_t buffer[65];
    uint16_t size = sizeof(buffer);
    uint32_t result = lora->receive(buffer, &size);

    if (result == RESULT_OK) {
        if (size < 1) {
            return;
        }

        uint8_t commandType = buffer[0];
        
        // Print received packet info
        Serial.println(F("\n========== LoRa Packet Received =========="));
        printRawData("Raw Data", buffer, size);
        Serial.print(F("Command Type: "));
        Serial.println(commandType);
        
        // Process based on command type
        switch(commandType) {
            case CMD_CHANGE_GATEWAY_MAC: {
                Serial.println(F(">>> Processing CMD_CHANGE_GATEWAY_MAC"));
                processChangeGatewayMacPacket(buffer, size);
                break; 
            }
            case CMD_CONTROL_DEVICE: {
                Serial.println(F(">>> Processing CMD_CONTROL_DEVICE"));
                processControlDevice(buffer, size);
                break;
            }
            case CMD_SCAN_REQUEST: {
                Serial.println(F(">>> Processing CMD_SCAN_REQUEST"));
                processScanRequest(buffer, size);
                break;
            }
            case CMD_DELETE_GATEWAY: {
                Serial.println(F(">>> Processing CMD_DELETE_GATEWAY"));
                processDeleteGateway(buffer, size);
                break;
            }

            default: {
                Serial.print(F("Unknown command type: "));
                Serial.println(commandType);
                break;
            }
        }
        
        Serial.println(F("=========================================\n"));
    }
}

void AppService::processChangeGatewayMacPacket(uint8_t* buffer, uint16_t size) {
    if (size < 1) {
        Serial.println(F("Error: Packet too small"));
        return;
    }
    
    // Decode ChangeGatewayMac (skip command byte)
    ChangeGatewayMac changeGatewayMac = {};
    ProtoCodec::decode(
        buffer + 1,
        size - 1,
        &changeGatewayMac,
        ChangeGatewayMac_fields,
        ChangeGatewayMac_field_count
    );
    
    // Check if nodeMac exists and matches
    if (changeGatewayMac.has_nodeMac) {
        Serial.print(F("Received NodeMac: "));
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", changeGatewayMac.nodeMac[i]);
        }
        Serial.println();
        
        if (isNodeMacMatch(changeGatewayMac.nodeMac)) {
            Serial.println(F("NodeMac MATCHED!"));

            bool saved = false;

            // Check if newGatewayMac exists
            if (changeGatewayMac.has_newGatewayMac) {
                Serial.print(F("New Gateway MAC: "));
                for (int i = 0; i < 6; i++) {
                    Serial.printf("%02X", changeGatewayMac.newGatewayMac[i]);
                }
                Serial.println();

                // Save to NVS using GatewayMacManager
                GatewayMacManager *gwMac = GatewayMacManager::getInstance();
                if (gwMac->saveGatewayMac(changeGatewayMac.newGatewayMac)) {
                    Serial.println(F("Gateway MAC saved to NVS successfully!"));
                    saved = true;
                } else {
                    Serial.println(F("Failed to save Gateway MAC to NVS"));
                }
            } else {
                Serial.println(F("Warning: newGatewayMac not found in packet"));
            }

            // Gửi phản hồi kết quả về Gateway
            sendChangeGatewayResponse(changeGatewayMac.nodeMac, saved);
        } else {
            Serial.println(F("NodeMac NOT matched. Ignoring packet."));
        }
    } else {
        Serial.println(F("Error: nodeMac not found in packet"));
    }
}

void AppService::sendChangeGatewayResponse(const uint8_t* nodeMac, bool success) {
    AppEngine* engine = AppEngine::getInstance();
    LoraService* lora = engine->getLoraService();

    if (!lora->isConnected()) {
        Serial.println(F("Cannot send ChangeGatewayResponse: LoRa not connected"));
        return;
    }

    ChangeGatewayResponse resp = {};
    resp.has_nodeMac = true;
    memcpy(resp.nodeMac, nodeMac, 6);
    resp.has_success = true;
    resp.success = success;

    uint8_t buffer[32];
    buffer[0] = CMD_CHANGE_GATEWAY_RESPONSE;

    uint16_t payloadSize = ProtoCodec::encode(
        buffer + 1,
        sizeof(buffer) - 1,
        &resp,
        ChangeGatewayResponse_fields,
        ChangeGatewayResponse_field_count);

    if (payloadSize == 0) {
        Serial.println(F("Encode ChangeGatewayResponse failed"));
        return;
    }

    uint16_t totalSize = 1 + payloadSize;

    if (lora->send(buffer, totalSize) == RESULT_OK) {
        Serial.print(F("Sent CMD_CHANGE_GATEWAY_RESPONSE success="));
        Serial.println(success ? F("true") : F("false"));
    } else {
        Serial.println(F("Failed to send CMD_CHANGE_GATEWAY_RESPONSE"));
    }
}

void AppService::processScanRequest(uint8_t* buffer, uint16_t size) {
    AppEngine* engine = AppEngine::getInstance();
    LoraService* lora = engine->getLoraService();

    if (!lora->isConnected()) {
        return;
    }

    // Giải mã sessionId (nếu có). Gateway phát lặp cùng sessionId trong 1 đợt
    // quét -> mỗi đợt chỉ đáp 1 lần để giảm nhiễu sóng. Vẫn dự phòng mất gói
    // chiều đi: nhận được BẤT KỲ gói nào trong đợt là đáp.
    ScanRequest req = {};
    if (size > 1) {
        ProtoCodec::decode(buffer + 1, size - 1, &req,
                           ScanRequest_fields, ScanRequest_field_count);
    }
    if (req.has_sessionId) {
        if ((int)req.sessionId == mLastScanSession) {
            Serial.println(F("Scan: da phan hoi dot nay, bo qua goi lap"));
            return;
        }
        mLastScanSession = (int)req.sessionId;   // đánh dấu đã đáp đợt này
    }

    // Nhiều Node cùng nhận 1 gói broadcast -> phản hồi cùng lúc sẽ đụng độ
    // trên sóng LoRa. Thêm độ trễ ngẫu nhiên nhỏ để giảm va chạm.
    delay(random(0, 500));

    ScanResponse resp = {};

    // NodeMac: LUÔN gửi (định danh chính của Node).
    memcpy(resp.nodeMac, mDeviceNodeMac, 6);
    resp.has_nodeMac = true;

    // GatewayMac: chỉ gửi nếu Node đã lưu Gateway MAC trong NVS.
    // Node chưa gắn Gateway -> bỏ trống gatewayMac nhưng vẫn báo nodeMac.
    uint8_t gatewayMac[6] = {};
    GatewayMacManager* gwMac = GatewayMacManager::getInstance();
    if (gwMac->loadGatewayMac(gatewayMac)) {
        memcpy(resp.gatewayMac, gatewayMac, 6);
        resp.has_gatewayMac = true;
    } else {
        Serial.println(F("Scan: Node chua co GatewayMac, chi gui NodeMac"));
    }

    uint8_t out[32];
    out[0] = CMD_SCAN_RESPONSE;

    uint16_t payloadSize = ProtoCodec::encode(
        out + 1,
        sizeof(out) - 1,
        &resp,
        ScanResponse_fields,
        ScanResponse_field_count);

    uint16_t totalSize = 1 + payloadSize;

    if (lora->send(out, totalSize) == RESULT_OK) {
        Serial.print(F("Sent CMD_SCAN_RESPONSE ("));
        Serial.print(totalSize);
        Serial.println(F(" bytes)"));
    } else {
        Serial.println(F("Failed to send CMD_SCAN_RESPONSE"));
    }
}

void AppService::processDeleteGateway(uint8_t* buffer, uint16_t size) {
    // Giải mã gatewayMac của Gateway đang bị xóa.
    DeleteGateway msg = {};
    if (size > 1) {
        ProtoCodec::decode(buffer + 1, size - 1, &msg,
                           DeleteGateway_fields, DeleteGateway_field_count);
    }
    if (!msg.has_gatewayMac) {
        Serial.println(F("Delete: thieu gatewayMac, bo qua"));
        return;
    }

    // Chỉ xóa nếu Node ĐANG gắn đúng Gateway này (so với GatewayMac đã lưu NVS).
    GatewayMacManager* gwMac = GatewayMacManager::getInstance();
    uint8_t storedMac[6] = {};
    if (!gwMac->loadGatewayMac(storedMac)) {
        Serial.println(F("Delete: Node chua gan Gateway nao, bo qua"));
        return;
    }
    if (memcmp(storedMac, msg.gatewayMac, 6) != 0) {
        Serial.println(F("Delete: GatewayMac khong khop, bo qua"));
        return;
    }

    // Trùng -> xóa liên kết: xóa GatewayMac khỏi NVS, về trạng thái chưa gắn.
    if (gwMac->resetGatewayMac()) {
        Serial.println(F("Delete: da xoa GatewayMac, Node ve trang thai chua gan"));
    } else {
        Serial.println(F("Delete: xoa GatewayMac that bai"));
    }
}

void AppService::printRawData(const char* label, uint8_t* data, uint16_t size) {
    Serial.print(label);
    Serial.print(F(" ("));
    Serial.print(size);
    Serial.print(F(" bytes): "));
    for (uint16_t i = 0; i < size; i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();
}

bool AppService::isNodeMacMatch(const uint8_t* receivedMac) {
    if (receivedMac == nullptr) {
        return false;
    }
    
    for (int i = 0; i < 6; i++) {
        if (mDeviceNodeMac[i] != receivedMac[i]) {
            return false;
        }
    }
    
    return true;
}

void AppService::processControlDevice(uint8_t* buffer, uint16_t size) {
    if (size < 1) {
        Serial.println(F("Error: Packet too small"));
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

    // Check if nodeMac exists and matches this device
    if (!controlDevice.has_nodeMac) {
        Serial.println(F("Error: nodeMac not found in packet"));
        isValid = false;
    } else {
        Serial.print(F("Received NodeMac: "));
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", controlDevice.nodeMac[i]);
        }
        Serial.println();

        if (!isNodeMacMatch(controlDevice.nodeMac)) {
            Serial.println(F("NodeMac NOT matched. Ignoring packet."));
            return;
        }
    }

    // Check if deviceId exists
    if (!controlDevice.has_deviceId) {
        Serial.println(F("Error: deviceId not found in packet"));
        isValid = false;
    } else {
        Serial.print(F("Device ID: "));
        Serial.println(controlDevice.deviceId);
    }

    // Check if action exists
    if (!controlDevice.has_action) {
        Serial.println(F("Error: action not found in packet"));
        isValid = false;
    } else {
        Serial.print(F("Action: "));
        Serial.println((uint8_t)controlDevice.action);
    }

    // If data is valid, send raw data to STM32 immediately
    if (isValid) {
        // Send raw buffer data (before decode) to STM32
        AppEngine* engine = AppEngine::getInstance();
        STM32Service* stm32 = engine->getSTM32Service();

        Serial.print(F("Sending control data to STM32 ("));
        Serial.print(size);
        Serial.print(F(" bytes): "));
        for (uint16_t i = 0; i < size; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();

        if (stm32->sendData(buffer, size) == RESULT_OK) {
            Serial.println(F("Control data sent to STM32 successfully"));
            sendReceiptAck(controlDevice);
        } else {
            Serial.println(F("Failed to send control data to STM32"));
        }
    } else {
        Serial.println(F("Invalid control device data. Data not sent to STM32."));
    }
}

// -----------------------------------------------------------------------
// ACK "đã nhận lệnh": Node tự dựng CMD_CONTROL_RESPONSE {nodeMac, deviceId,
// state = action yêu cầu} gửi LoRa về Gateway NGAY khi nhận lệnh hợp lệ —
// không chờ phản hồi relay từ STM32 (chặng UART ngược hay rớt, xem spec
// 2026-07-02-node-receipt-ack-design.md). Gateway nhận gói này sẽ resolve
// campaign + publish node_response lên MQTT như cũ.
// -----------------------------------------------------------------------
void AppService::sendReceiptAck(const ControlDevice& cmd) {
    // Dựng gói CMD_CONTROL_RESPONSE 1 lần.
    ControlResponse resp = {};
    resp.has_nodeMac = true;
    memcpy(resp.nodeMac, mDeviceNodeMac, 6);
    resp.has_deviceId = true;
    resp.deviceId = cmd.deviceId;
    resp.has_state = true;
    // state = 1 nếu action ON, 0 nếu OFF — khớp contract cũ (0/1) mà backend
    // handleNodeResponse dùng (state===1 → on). action enum: ON=1, OFF=2.
    resp.state = (cmd.action == ControlDevice::CONTROL_ACTION_ON) ? 1 : 0;

    uint8_t packet[32];
    packet[0] = CMD_CONTROL_RESPONSE;
    uint16_t payloadSize = ProtoCodec::encode(
        packet + 1, sizeof(packet) - 1, &resp,
        ControlResponse_fields, ControlResponse_field_count);

    LoraService* lora = AppEngine::getInstance()->getLoraService();
    if (!lora || !lora->isConnected()) {
        Serial.println(F("Cannot send receipt ACK: LoRa not connected"));
        return;
    }

    // Phát LẶP LẠI ACK_REPEAT lần, trải dài ACK_GAP để phủ ≥1-2 chu kỳ resend của Gateway.
    // Lý do (chốt từ log + đo 2026-07-02): LoRa half-duplex nên trong lúc Gateway đang phát
    // gói lệnh (resend mỗi 2.5s, mỗi lần chiếm E32 vài trăm ms) thì E32 Gateway ĐIẾC, không
    // thu được. Đo thực tế: Node hay chỉ bắt được gói lệnh ở lần resend cuối rồi mới ACK, nên
    // 3 gói/1.5s có thể lọt trọn vào 1 cửa sổ Gateway đang bận → rớt. Nâng lên 5 gói trải
    // ~4s (600ms-1s giữa mỗi gói) = qua gần 2 chu kỳ resend → gần như chắc có ≥1 gói rơi vào
    // khoảng lặng Gateway rảnh thu. Gateway nhận 1 gói là resolve campaign (c.active=false),
    // các gói ACK dư sau bị bỏ vô hại. Đánh đổi: Node bị delay() chặn ~4s/lệnh (sensor trễ 1
    // nhịp, chấp nhận được vì lúc đó chỉ phục vụ đúng 1 lệnh điều khiển).
    const uint8_t ACK_REPEAT = 5;
    for (uint8_t i = 0; i < ACK_REPEAT; i++) {
        if (lora->send(packet, (uint16_t)(1 + payloadSize)) == RESULT_OK) {
            Serial.printf("Receipt ACK sent to Gateway (%d/%d)\n", i + 1, ACK_REPEAT);
        } else {
            Serial.printf("Failed to send receipt ACK (%d/%d)\n", i + 1, ACK_REPEAT);
        }
        if (i < ACK_REPEAT - 1) {
            delay(600 + (unsigned long)random(0, 401)); // 600-1000ms, jitter phá cộng hưởng
        }
    }
}

