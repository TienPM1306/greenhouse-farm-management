#include "app_service.h"
#include "app_engine.h"
#include "mqtt.h"
#include "lora_service.h"
#include "common/command.h"
#include "common/protoc.h"

AppService *AppService::mInstance = nullptr;

AppService *AppService::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new AppService();
    }
    return mInstance;
}

AppService::AppService() {}

uint32_t AppService::initialize()
{
    Serial.println(F("AppService initialized"));
    return RESULT_OK;
}

void AppService::run()
{
    checkRetry();
    tickControl();
    tickScan();
    tickDeleteGateway();
    receiveNodeData();
    sendOnlineHeartbeat();
    vTaskDelay(pdMS_TO_TICKS(100));
}

// -----------------------------------------------------------------------
// Scan campaign: phát broadcast CMD_SCAN_REQUEST mỗi 2s, tối đa 5 gói (10s)
// -----------------------------------------------------------------------
void AppService::startScan()
{
    // Chỉ đặt cờ; việc phát LoRa do tickScan() (service task) thực hiện để
    // tránh truy cập LoRa từ task MQTT.
    mScanRequested = true;
}

void AppService::tickScan()
{
    // Nhận yêu cầu mới -> (re)khởi động chiến dịch, ép phát ngay gói đầu tiên
    if (mScanRequested) {
        mScanRequested = false;
        mScanActive    = true;
        mScanCount     = 0;
        mScanResultCount = 0;                 // bắt đầu đợt mới -> xóa kết quả đợt trước
        mScanSession++;                       // ID mới: Node sẽ đáp lại 1 lần cho đợt này
        mScanLastSent  = millis() - SCAN_INTERVAL;
    }

    if (!mScanActive) return;
    if (millis() - mScanLastSent < SCAN_INTERVAL) return;

    if (mScanCount >= SCAN_MAX_PACKETS) {
        mScanActive = false;
        Serial.println(F("[SVC] Scan campaign finished"));
        flushScanResults();
        return;
    }

    LoraService *lora = AppEngine::getInstance()->getLoraService();
    if (!lora || !lora->connected()) {
        mScanActive = false;
        return;
    }

    // Gói: [CMD_SCAN_REQUEST][ScanRequest{sessionId}]. Cả 5 gói cùng sessionId.
    ScanRequest req = {};
    req.has_sessionId = true;
    req.sessionId     = mScanSession;

    uint8_t packet[8];
    packet[0] = CMD_SCAN_REQUEST;
    uint16_t n = ProtoCodec::encode(packet + 1, sizeof(packet) - 1, &req,
                                    ScanRequest_fields, ScanRequest_field_count);
    uint16_t total = 1 + n;

    mScanLastSent = millis();
    mScanCount++;

    if (lora->send(packet, total) == RESULT_OK) {
        Serial.printf("[SVC] Scan request sent (%d/%d) session=%u\n",
                      mScanCount, (int)SCAN_MAX_PACKETS, mScanSession);
    } else {
        Serial.println(F("[SVC] Scan request send failed"));
    }
}

// -----------------------------------------------------------------------
// Delete-gateway campaign: broadcast CMD_DELETE_GATEWAY (kèm gatewayMac của
// chính Gateway) 5 gói / 10s để Node gắn Gateway này tự xóa liên kết.
// -----------------------------------------------------------------------
void AppService::startDeleteGateway()
{
    mDeleteRequested = true;
}

void AppService::tickDeleteGateway()
{
    if (mDeleteRequested) {
        mDeleteRequested = false;
        mDeleteActive    = true;
        mDeleteCount     = 0;
        mDeleteLastSent  = millis() - SCAN_INTERVAL;
    }

    if (!mDeleteActive) return;
    if (millis() - mDeleteLastSent < SCAN_INTERVAL) return;

    if (mDeleteCount >= SCAN_MAX_PACKETS) {
        mDeleteActive = false;
        Serial.println(F("[SVC] Delete-gateway campaign finished"));
        return;
    }

    LoraService *lora = AppEngine::getInstance()->getLoraService();
    if (!lora || !lora->connected()) {
        mDeleteActive = false;
        return;
    }

    uint8_t gatewayMac[6];
    if (!getMacAddress(gatewayMac)) {
        Serial.println(F("[SVC] Delete: cannot read gateway MAC"));
        mDeleteActive = false;
        return;
    }

    DeleteGateway msg = {};
    msg.has_gatewayMac = true;
    memcpy(msg.gatewayMac, gatewayMac, 6);

    uint8_t packet[16];
    packet[0] = CMD_DELETE_GATEWAY;
    uint16_t n = ProtoCodec::encode(packet + 1, sizeof(packet) - 1, &msg,
                                    DeleteGateway_fields, DeleteGateway_field_count);
    uint16_t total = 1 + n;

    mDeleteLastSent = millis();
    mDeleteCount++;

    if (lora->send(packet, total) == RESULT_OK) {
        Serial.printf("[SVC] Delete-gateway broadcast (%d/%d)\n",
                      mDeleteCount, (int)SCAN_MAX_PACKETS);
    } else {
        Serial.println(F("[SVC] Delete-gateway send failed"));
    }
}

// -----------------------------------------------------------------------
// Register a pending LoRa command to be retried if no response arrives
// -----------------------------------------------------------------------
void AppService::registerPending(uint8_t cmdType, const uint8_t *payload,
                                  uint16_t len, const uint8_t *nodeMac,
                                  uint8_t expectResp)
{
    if (len > sizeof(mPending.payload)) {
        Serial.println(F("[SVC] registerPending: payload too large"));
        return;
    }
    mPending.active     = true;
    mPending.cmdType    = cmdType;
    mPending.payloadLen = len;
    mPending.expectResp = expectResp;
    mPending.sentAt     = millis();
    mPending.attempts   = 1;
    memcpy(mPending.payload, payload, len);
    memcpy(mPending.nodeMac, nodeMac, 6);
    Serial.printf("[SVC] Pending registered: cmd=0x%02X expect=0x%02X attempt=1\n",
                  cmdType, expectResp);
}

// -----------------------------------------------------------------------
// Resolve pending when matching response arrives
// -----------------------------------------------------------------------
void AppService::resolvePending(uint8_t respType, const uint8_t *nodeMac)
{
    if (!mPending.active) return;
    if (mPending.expectResp != respType) return;
    if (memcmp(mPending.nodeMac, nodeMac, 6) != 0) return;

    Serial.printf("[SVC] Pending resolved: cmd=0x%02X after %d attempt(s)\n",
                  mPending.cmdType, mPending.attempts);
    mPending.active = false;
}

// -----------------------------------------------------------------------
// Retry logic: resend if no response received within RETRY_TIMEOUT
// -----------------------------------------------------------------------
void AppService::checkRetry()
{
    if (!mPending.active) return;
    if (millis() - mPending.sentAt < RETRY_TIMEOUT) return;

    if (mPending.attempts >= RETRY_MAX) {
        Serial.printf("[SVC] Retry exhausted for cmd=0x%02X after %d attempts\n",
                      mPending.cmdType, mPending.attempts);
        mPending.active = false;
        return;
    }

    LoraService *lora = AppEngine::getInstance()->getLoraService();
    if (!lora || !lora->connected()) {
        mPending.active = false;
        return;
    }

    uint8_t packet[1 + 64];
    packet[0] = mPending.cmdType;
    memcpy(packet + 1, mPending.payload, mPending.payloadLen);

    mPending.attempts++;
    mPending.sentAt = millis();

    Serial.printf("[SVC] Retrying cmd=0x%02X (attempt %d/%d)\n",
                  mPending.cmdType, mPending.attempts, (int)RETRY_MAX);

    if (lora->send(packet, static_cast<uint16_t>(1 + mPending.payloadLen)) != RESULT_OK) {
        Serial.println(F("[SVC] Retry send failed"));
    }
}

// -----------------------------------------------------------------------
// Control-device campaign: đăng ký chiến dịch gửi CMD_CONTROL_DEVICE.
// Tìm 1 slot rảnh (hoặc slot đang active cùng nodeMac+deviceId để thay thế
// lệnh cũ bằng lệnh mới nhất, ví dụ user bấm on rồi off liên tiếp).
// -----------------------------------------------------------------------
void AppService::registerControlCampaign(const uint8_t *payload, uint16_t len,
                                          const uint8_t *nodeMac, uint8_t deviceId)
{
    if (len > sizeof(mControlCampaigns[0].payload)) {
        Serial.println(F("[SVC] registerControlCampaign: payload too large"));
        return;
    }

    int slot = -1;
    for (uint8_t i = 0; i < CONTROL_MAX_SLOTS; i++) {
        if (!mControlCampaigns[i].active) { slot = i; break; }
        if (memcmp(mControlCampaigns[i].nodeMac, nodeMac, 6) == 0 &&
            mControlCampaigns[i].deviceId == deviceId) {
            slot = i; break;
        }
    }
    if (slot < 0) {
        // Hết slot rảnh: ghi đè slot cũ nhất (sentCount lớn nhất = gần hết hạn).
        uint8_t maxSent = 0;
        for (uint8_t i = 0; i < CONTROL_MAX_SLOTS; i++) {
            if (mControlCampaigns[i].sentCount >= maxSent) {
                maxSent = mControlCampaigns[i].sentCount;
                slot = i;
            }
        }
        Serial.printf("[SVC] Control campaign: het slot, ghi de slot %d\n", slot);
    }

    ControlCampaign &c = mControlCampaigns[slot];
    c.active     = true;
    c.payloadLen = len;
    c.deviceId   = deviceId;
    c.sentCount  = 1;
    // Gói đầu tiên đã được loraForward() gửi ngay khi nhận lệnh MQTT;
    // tickControl() chỉ phát các gói lặp lại tiếp theo.
    c.lastSent   = millis();
    memcpy(c.payload, payload, len);
    memcpy(c.nodeMac, nodeMac, 6);
    Serial.printf("[SVC] Control campaign registered: slot=%d deviceId=%d (1/%d)\n",
                  slot, deviceId, (int)CONTROL_MAX_PACKETS);
}

// -----------------------------------------------------------------------
// Control-device campaign: phát lặp lại mỗi CONTROL_INTERVAL, tối đa
// CONTROL_MAX_PACKETS gói; dừng ngay khi handleControlResponse() resolve.
// Lặp qua từng slot — chỉ gửi tối đa 1 gói/lượt gọi (module LoRa half-duplex
// không thể gửi 2 gói đồng thời); các slot khác chờ đến lượt lastSent của nó.
// -----------------------------------------------------------------------
void AppService::tickControl()
{
    LoraService *lora = AppEngine::getInstance()->getLoraService();

    for (uint8_t i = 0; i < CONTROL_MAX_SLOTS; i++) {
        ControlCampaign &c = mControlCampaigns[i];
        if (!c.active) continue;

        // Jitter ngẫu nhiên: Node tự gửi sensor data mỗi 5000ms cố định, module
        // E32 half-duplex nên "điếc" tạm thời lúc đó. Nếu Gateway resend đúng
        // chu kỳ cố định, gói có thể liên tục rơi vào cửa sổ điếc đó. Thêm
        // +/-300ms để phá vỡ cộng hưởng giữa 2 chu kỳ cố định.
        unsigned long interval = CONTROL_INTERVAL + random(-300, 301);
        if (millis() - c.lastSent < interval) continue;

        if (c.sentCount >= CONTROL_MAX_PACKETS) {
            Serial.printf("[SVC] Control campaign slot=%d exhausted after %d packet(s), no ACK\n",
                          i, c.sentCount);

            char macStr[13];
            formatMacAddress(c.nodeMac, macStr, sizeof(macStr));
            char json[128];
            snprintf(json, sizeof(json),
                     "{\"cmd\":%d,\"nodeMac\":\"%s\",\"deviceId\":%d,\"state\":-1}",
                     CMD_CONTROL_RESPONSE, macStr, c.deviceId);

            MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
            if (mqtt && mqtt->isConnected()) {
                mqtt->publishResponse(json);
                Serial.printf("[SVC] Control campaign slot=%d timeout published: device=%d\n",
                              i, c.deviceId);
            }

            c.active = false;
            continue;
        }

        if (!lora || !lora->connected()) {
            c.active = false;
            continue;
        }

        uint8_t packet[1 + 64];
        packet[0] = CMD_CONTROL_DEVICE;
        memcpy(packet + 1, c.payload, c.payloadLen);

        c.sentCount++;
        c.lastSent = millis();

        Serial.printf("[SVC] Control campaign slot=%d resend (%d/%d)\n",
                      i, c.sentCount, (int)CONTROL_MAX_PACKETS);

        if (lora->send(packet, static_cast<uint16_t>(1 + c.payloadLen)) != RESULT_OK) {
            Serial.println(F("[SVC] Control campaign send failed"));
        }
        // Chỉ gửi 1 gói mỗi lượt gọi tickControl() để tránh dồn nhiều gói LoRa
        // liên tiếp không có khoảng nghỉ giữa các slot khác nhau.
        break;
    }
}

// -----------------------------------------------------------------------
// Receive and dispatch LoRa packets from Nodes
// -----------------------------------------------------------------------
void AppService::receiveNodeData()
{
    LoraService *lora = AppEngine::getInstance()->getLoraService();
    if (!lora || !lora->connected()) return;

    uint8_t  buffer[128] = {0};
    uint16_t size        = sizeof(buffer);

    if (lora->receive(buffer, &size) != RESULT_OK) return;

    if (size < 1) {
        Serial.println(F("[SVC] Empty packet"));
        return;
    }

    Serial.print(F("[SVC] LoRa RX: "));
    for (uint16_t i = 0; i < size; i++) Serial.printf("%02X ", buffer[i]);
    Serial.println();

    uint8_t  cmd     = buffer[0];
    uint8_t  *payload = buffer + 1;
    uint16_t pLen    = size - 1;

    switch (cmd) {
        case CMD_SYNC_SENSOR_DATA:
            handleSensorData(payload, pLen);
            break;
        case CMD_CONTROL_RESPONSE:
            handleControlResponse(payload, pLen);
            break;
        case CMD_STATE_RESPONSE:
            handleStateResponse(payload, pLen);
            break;
        case CMD_ALL_STATES_RESPONSE:
            handleAllStatesResponse(payload, pLen);
            break;
        case CMD_CHANGE_TIME_RESPONSE:
            handleChangeTimeResponse(payload, pLen);
            break;
        case CMD_CHANGE_GATEWAY_RESPONSE:
            handleChangeGatewayResponse(payload, pLen);
            break;
        case CMD_SCAN_RESPONSE:
            handleScanResponse(payload, pLen);
            break;
        default:
            Serial.printf("[SVC] Unknown LoRa cmd: 0x%02X\n", cmd);
            break;
    }
}

// -----------------------------------------------------------------------
// Online heartbeat: publish "1" every HEARTBEAT_INTERVAL ms
// -----------------------------------------------------------------------
void AppService::sendOnlineHeartbeat()
{
    unsigned long now = millis();
    if (now - mLastHeartbeat < HEARTBEAT_INTERVAL) return;

    mLastHeartbeat = now;
    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (mqtt && mqtt->isConnected()) {
        mqtt->publishOnline();
        Serial.println(F("[SVC] Online heartbeat sent"));
    }
}

// -----------------------------------------------------------------------
// CMD_SYNC_SENSOR_DATA (0x01)
// -----------------------------------------------------------------------
void AppService::handleSensorData(const uint8_t *payload, uint16_t size)
{
    if (size == 0) {
        Serial.println(F("[SVC] SensorData: empty payload"));
        return;
    }

    SensorData sd = {};
    ProtoCodec::decode((uint8_t *)payload, size, &sd,
                       SensorData_fields, SensorData_field_count);

    if (!sd.identity.has_gatewayMac) {
        Serial.println(F("[SVC] SensorData: missing gatewayMac"));
        return;
    }

    uint8_t localMac[6];
    if (!getMacAddress(localMac)) {
        Serial.println(F("[SVC] SensorData: unable to read local MAC"));
        return;
    }

    if (memcmp(localMac, sd.identity.gatewayMac, 6) != 0) {
        Serial.println(F("[SVC] SensorData: gatewayMac mismatch"));
        return;
    }

    if (!AppEngine::getInstance()->isGatewayRegistered()) {
        Serial.println(F("[SVC] SensorData: gateway not registered, skip"));
        return;
    }

    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (!mqtt || !mqtt->isConnected()) {
        Serial.println(F("[SVC] SensorData: MQTT not connected"));
        return;
    }

    char nodeMacStr[13];
    formatMacAddress(sd.identity.nodeMac, nodeMacStr, sizeof(nodeMacStr));

    char mqttPayload[192];
    snprintf(mqttPayload, sizeof(mqttPayload),
             "{\"nodeMac\":\"%s\""
             ",\"light\":%.2f"
             ",\"temperature\":%.2f"
             ",\"humidity\":%.2f}",
             nodeMacStr,
             sd.data.has_light       ? sd.data.light       : 0.0f,
             sd.data.has_temperature ? sd.data.temperature : 0.0f,
             sd.data.has_humidity    ? sd.data.humidity    : 0.0f);

    if (mqtt->publishSensor(mqttPayload) == RESULT_OK) {
        Serial.println(F("[SVC] Sensor data published"));
    }
}


// -----------------------------------------------------------------------
// CMD_CONTROL_RESPONSE (0x0A)
// -----------------------------------------------------------------------
void AppService::handleControlResponse(const uint8_t *payload, uint16_t size)
{
    ControlResponse resp = {};
    ProtoCodec::decode((uint8_t *)payload, size, &resp,
                       ControlResponse_fields, ControlResponse_field_count);

    if (!resp.has_nodeMac) return;

    char macStr[13];
    formatMacAddress(resp.nodeMac, macStr, sizeof(macStr));

    char json[128];
    snprintf(json, sizeof(json),
             "{\"cmd\":%d,\"nodeMac\":\"%s\",\"deviceId\":%d,\"state\":%d}",
             CMD_CONTROL_RESPONSE, macStr, resp.deviceId, resp.state);

    for (uint8_t i = 0; i < CONTROL_MAX_SLOTS; i++) {
        ControlCampaign &c = mControlCampaigns[i];
        if (c.active && c.deviceId == resp.deviceId &&
            memcmp(c.nodeMac, resp.nodeMac, 6) == 0) {
            Serial.printf("[SVC] Control campaign slot=%d resolved after %d packet(s)\n",
                          i, c.sentCount);
            c.active = false;
            break;
        }
    }

    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (mqtt && mqtt->isConnected()) {
        mqtt->publishResponse(json);
        Serial.printf("[SVC] ControlResponse published: device=%d state=%d\n",
                      resp.deviceId, resp.state);
    }
}

// -----------------------------------------------------------------------
// CMD_STATE_RESPONSE (0x0B)
// -----------------------------------------------------------------------
void AppService::handleStateResponse(const uint8_t *payload, uint16_t size)
{
    StateResponse resp = {};
    ProtoCodec::decode((uint8_t *)payload, size, &resp,
                       StateResponse_fields, StateResponse_field_count);

    if (!resp.has_nodeMac) return;

    resolvePending(CMD_STATE_RESPONSE, resp.nodeMac);

    char macStr[13];
    formatMacAddress(resp.nodeMac, macStr, sizeof(macStr));

    char json[128];
    snprintf(json, sizeof(json),
             "{\"cmd\":%d,\"nodeMac\":\"%s\",\"deviceId\":%d,\"state\":%d}",
             CMD_STATE_RESPONSE, macStr, resp.deviceId, resp.state);

    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (mqtt && mqtt->isConnected()) {
        mqtt->publishResponse(json);
    }
}

// -----------------------------------------------------------------------
// CMD_ALL_STATES_RESPONSE (0x0C)
// -----------------------------------------------------------------------
void AppService::handleAllStatesResponse(const uint8_t *payload, uint16_t size)
{
    AllStatesResponse resp = {};
    ProtoCodec::decode((uint8_t *)payload, size, &resp,
                       AllStatesResponse_fields, AllStatesResponse_field_count);

    if (!resp.has_nodeMac) return;

    resolvePending(CMD_ALL_STATES_RESPONSE, resp.nodeMac);

    char macStr[13];
    formatMacAddress(resp.nodeMac, macStr, sizeof(macStr));

    char json[160];
    snprintf(json, sizeof(json),
             "{\"cmd\":%d,\"nodeMac\":\"%s\",\"states\":%u}",
             CMD_ALL_STATES_RESPONSE, macStr, resp.deviceStates);

    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (mqtt && mqtt->isConnected()) {
        mqtt->publishResponse(json);
        Serial.printf("[SVC] AllStatesResponse: states=0x%04X\n", resp.deviceStates);
    }
}

// -----------------------------------------------------------------------
// CMD_CHANGE_TIME_RESPONSE (0x0E)
// -----------------------------------------------------------------------
void AppService::handleChangeTimeResponse(const uint8_t *payload, uint16_t size)
{
    ChangeTimeResponse resp = {};
    ProtoCodec::decode((uint8_t *)payload, size, &resp,
                       ChangeTimeResponse_fields, ChangeTimeResponse_field_count);

    if (!resp.has_nodeMac) return;

    resolvePending(CMD_CHANGE_TIME_RESPONSE, resp.nodeMac);

    char macStr[13];
    formatMacAddress(resp.nodeMac, macStr, sizeof(macStr));

    char json[128];
    snprintf(json, sizeof(json),
             "{\"cmd\":%d,\"nodeMac\":\"%s\",\"success\":%s}",
             CMD_CHANGE_TIME_RESPONSE, macStr, resp.success ? "true" : "false");

    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (mqtt && mqtt->isConnected()) {
        mqtt->publishResponse(json);
    }
}

// -----------------------------------------------------------------------
// CMD_CHANGE_GATEWAY_RESPONSE (0x0F)
// -----------------------------------------------------------------------
void AppService::handleChangeGatewayResponse(const uint8_t *payload, uint16_t size)
{
    ChangeGatewayResponse resp = {};
    ProtoCodec::decode((uint8_t *)payload, size, &resp,
                       ChangeGatewayResponse_fields, ChangeGatewayResponse_field_count);

    if (!resp.has_nodeMac) return;

    resolvePending(CMD_CHANGE_GATEWAY_RESPONSE, resp.nodeMac);

    char macStr[13];
    formatMacAddress(resp.nodeMac, macStr, sizeof(macStr));

    char json[128];
    snprintf(json, sizeof(json),
             "{\"cmd\":%d,\"nodeMac\":\"%s\",\"success\":%s}",
             CMD_CHANGE_GATEWAY_RESPONSE, macStr, resp.success ? "true" : "false");

    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (mqtt && mqtt->isConnected()) {
        mqtt->publishResponse(json);
    }
}

// -----------------------------------------------------------------------
// CMD_SCAN_RESPONSE (0x0D) — Node báo danh tính khi được quét
// nodeMac luôn có; gatewayMac chỉ có nếu Node đã gắn Gateway.
// Gom vào buffer (KHÔNG publish ngay) — chờ campaign kết thúc, flushScanResults()
// sẽ publish 1 lần dạng JSON array.
// -----------------------------------------------------------------------
void AppService::handleScanResponse(const uint8_t *payload, uint16_t size)
{
    if (!mScanActive) {
        Serial.println(F("[SVC] ScanResponse: campaign đã kết thúc, bỏ qua (tránh lẫn vào đợt sau)"));
        return;
    }

    ScanResponse resp = {};
    ProtoCodec::decode((uint8_t *)payload, size, &resp,
                       ScanResponse_fields, ScanResponse_field_count);

    if (!resp.has_nodeMac) {
        Serial.println(F("[SVC] ScanResponse: missing nodeMac"));
        return;
    }

    addScanResult(resp.nodeMac, resp.has_gatewayMac, resp.gatewayMac);

    char nodeStr[13];
    formatMacAddress(resp.nodeMac, nodeStr, sizeof(nodeStr));
    Serial.printf("[SVC] ScanResponse buffered: node=%s gw=%s (total=%d)\n",
                  nodeStr, resp.has_gatewayMac ? "yes" : "none", mScanResultCount);
}

// -----------------------------------------------------------------------
// Thêm 1 kết quả scan vào buffer, bỏ qua nếu node đã có trong đợt này
// (Node có thể trả lời nhiều lần trong 1 campaign nếu bắt được >1 gói).
// -----------------------------------------------------------------------
void AppService::addScanResult(const uint8_t *nodeMac, bool hasGatewayMac,
                                const uint8_t *gatewayMac)
{
    for (uint8_t i = 0; i < mScanResultCount; i++) {
        if (memcmp(mScanResults[i].nodeMac, nodeMac, 6) == 0) return; // đã ghi nhận
    }

    if (mScanResultCount >= SCAN_MAX_RESULTS) {
        Serial.println(F("[SVC] ScanResult buffer full, bỏ qua node mới"));
        return;
    }

    ScanResult &r = mScanResults[mScanResultCount++];
    memcpy(r.nodeMac, nodeMac, 6);
    r.hasGatewayMac = hasGatewayMac;
    if (hasGatewayMac) memcpy(r.gatewayMac, gatewayMac, 6);
}

// -----------------------------------------------------------------------
// Publish 1 lần dạng JSON array tất cả node tìm được trong campaign vừa xong.
// Không publish gì nếu không tìm thấy node nào.
// -----------------------------------------------------------------------
void AppService::flushScanResults()
{
    if (mScanResultCount == 0) {
        Serial.println(F("[SVC] Scan: không tìm thấy node nào"));
        return;
    }

    MQTTServer *mqtt = AppEngine::getInstance()->getMqttServer();
    if (!mqtt || !mqtt->isConnected()) {
        Serial.println(F("[SVC] flushScanResults: MQTT không kết nối"));
        mScanResultCount = 0;
        return;
    }

    char json[MQTT_MAX_PAYLOAD_LENGTH];
    size_t pos = 0;
    pos += snprintf(json + pos, sizeof(json) - pos,
                     "{\"cmd\":%d,\"nodes\":[", CMD_SCAN_RESPONSE);

    for (uint8_t i = 0; i < mScanResultCount; i++) {
        char nodeStr[13];
        formatMacAddress(mScanResults[i].nodeMac, nodeStr, sizeof(nodeStr));

        if (mScanResults[i].hasGatewayMac) {
            char gwStr[13];
            formatMacAddress(mScanResults[i].gatewayMac, gwStr, sizeof(gwStr));
            pos += snprintf(json + pos, sizeof(json) - pos,
                             "%s{\"nodeMac\":\"%s\",\"gatewayMac\":\"%s\"}",
                             i == 0 ? "" : ",", nodeStr, gwStr);
        } else {
            pos += snprintf(json + pos, sizeof(json) - pos,
                             "%s{\"nodeMac\":\"%s\",\"gatewayMac\":null}",
                             i == 0 ? "" : ",", nodeStr);
        }
    }
    pos += snprintf(json + pos, sizeof(json) - pos, "]}");

    if (mqtt->publishResponse(json) == RESULT_OK) {
        Serial.printf("[SVC] ScanResponse published: %d node(s)\n", mScanResultCount);
    } else {
        Serial.println(F("[SVC] flushScanResults: publishResponse FAILED, kết quả scan bị mất"));
    }

    mScanResultCount = 0;
}
