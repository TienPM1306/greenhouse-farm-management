#ifndef APP_SERVICE_H
#define APP_SERVICE_H

#include "common.h"
#include "common/message.h"

class AppService
{
public:
    static AppService *getInstance();

    uint32_t initialize();
    void     run();

private:
    AppService();
    ~AppService() = default;
    AppService(const AppService &) = delete;
    AppService &operator=(const AppService &) = delete;

    static AppService *mInstance;

    unsigned long mLastHeartbeat = 0;
    static constexpr unsigned long HEARTBEAT_INTERVAL = 180000UL;

    // ---------------------------------------------------------------
    // Retry tracking: one pending LoRa command at a time
    // ---------------------------------------------------------------
    struct PendingCommand {
        bool     active      = false;
        uint8_t  cmdType     = 0;
        uint8_t  payload[64] = {};
        uint16_t payloadLen  = 0;
        uint8_t  nodeMac[6]  = {};
        uint8_t  expectResp  = 0;
        unsigned long sentAt = 0;
        uint8_t  attempts    = 0;
    };
    PendingCommand mPending;

    static constexpr uint8_t       RETRY_MAX     = 3;
    static constexpr unsigned long  RETRY_TIMEOUT = 4000UL;

    // ---------------------------------------------------------------
    // Control-device campaign: CMD_CONTROL_DEVICE dùng đường riêng,
    // phát tối đa CONTROL_MAX_PACKETS gói cách nhau CONTROL_INTERVAL
    // (10 gói / 15s), dừng ngay khi nhận CMD_CONTROL_RESPONSE khớp
    // nodeMac+deviceId. Nhiều slot song song để bật/tắt nhiều thiết bị
    // cùng lúc (mỗi thiết bị gửi 1 lệnh CMD_CONTROL_DEVICE riêng từ
    // backend) không bị ghi đè/bỏ ngang lẫn nhau.
    // ---------------------------------------------------------------
    struct ControlCampaign {
        bool     active      = false;
        uint8_t  payload[64] = {};
        uint16_t payloadLen  = 0;
        uint8_t  nodeMac[6]  = {};
        uint8_t  deviceId    = 0;
        unsigned long lastSent = 0;
        uint8_t  sentCount    = 0;
    };
    static constexpr uint8_t CONTROL_MAX_SLOTS = 4;
    ControlCampaign mControlCampaigns[CONTROL_MAX_SLOTS];

    // 2026-07-02: giãn 1500->2500ms + giảm 10->5 gói. Root cause đã xác định: phát dồn
    // 10 gói/1.5s làm E32 Gateway "điếc" (RX câm suốt campaign, hồi sau ~60s) → nuốt cả
    // receipt-ACK lẫn sensor. Node nay ACK lặp 3 lần nên chiều xuống vẫn chắc; giãn nhịp
    // cho E32 Gateway ~2.5s im giữa mỗi lần phát để hồi + kịp nghe ACK. Tổng ~12.5s <
    // PENDING_FALLBACK_MS=20000 phía web.
    static constexpr unsigned long CONTROL_INTERVAL    = 2500UL;
    static constexpr uint8_t       CONTROL_MAX_PACKETS = 5;

    // ---------------------------------------------------------------
    // Scan campaign: broadcast CMD_SCAN_REQUEST mỗi 2s trong 10s (5 gói)
    // ---------------------------------------------------------------
    volatile bool mScanRequested = false;   // đặt bởi MQTT task, đọc bởi service task
    bool          mScanActive    = false;
    uint8_t       mScanCount     = 0;
    uint8_t       mScanSession   = 0;      // ID đợt quét hiện tại (tăng mỗi đợt)
    unsigned long mScanLastSent  = 0;
    static constexpr unsigned long SCAN_INTERVAL    = 2000UL;
    // 2026-07-02: 5->9 gói (campaign ~18s). RF chập chờn khiến Node hay không nghe được yêu
    // cầu quét trong 5 gói/10s → sót node. Nhiều gói hơn = nhiều cơ hội Node bắt được ≥1 gói
    // và đáp. Frontend chờ tương ứng 20s (checkAvailable trong gateway-details.html).
    static constexpr uint8_t       SCAN_MAX_PACKETS = 9;

    // Gom kết quả ScanResponse trong suốt campaign, publish 1 lần khi kết thúc
    // (thay vì publish ngay từng node) — tối đa SCAN_MAX_RESULTS node/đợt.
    struct ScanResult {
        uint8_t nodeMac[6];
        bool    hasGatewayMac;
        uint8_t gatewayMac[6];
    };
    // 8 node tối đa để JSON tổng hợp không vượt MQTT_MAX_PAYLOAD_LENGTH (512B, xem mqtt.h)
    static constexpr uint8_t SCAN_MAX_RESULTS = 8;
    ScanResult mScanResults[SCAN_MAX_RESULTS];
    uint8_t    mScanResultCount = 0;

    void addScanResult(const uint8_t *nodeMac, bool hasGatewayMac, const uint8_t *gatewayMac);
    void flushScanResults();

    // ---------------------------------------------------------------
    // Delete-gateway campaign: broadcast CMD_DELETE_GATEWAY 5 gói / 10s
    // (dùng lại SCAN_INTERVAL / SCAN_MAX_PACKETS)
    // ---------------------------------------------------------------
    volatile bool mDeleteRequested = false;
    bool          mDeleteActive    = false;
    uint8_t       mDeleteCount     = 0;
    unsigned long mDeleteLastSent  = 0;

    void receiveNodeData();
    void sendOnlineHeartbeat();
    void checkRetry();
    void resolvePending(uint8_t respType, const uint8_t *nodeMac);
    void tickScan();
    void tickDeleteGateway();
    void tickControl();

    void handleSensorData(const uint8_t *payload, uint16_t size);
    void handleControlResponse(const uint8_t *payload, uint16_t size);
    void handleStateResponse(const uint8_t *payload, uint16_t size);
    void handleAllStatesResponse(const uint8_t *payload, uint16_t size);
    void handleChangeTimeResponse(const uint8_t *payload, uint16_t size);
    void handleChangeGatewayResponse(const uint8_t *payload, uint16_t size);
    void handleScanResponse(const uint8_t *payload, uint16_t size);

public:
    void registerPending(uint8_t cmdType, const uint8_t *payload, uint16_t len,
                         const uint8_t *nodeMac, uint8_t expectResp);

    // Đăng ký chiến dịch gửi CMD_CONTROL_DEVICE (10 gói / 10s, dừng sớm nếu có ACK).
    // Mỗi thiết bị (nodeMac+deviceId) dùng 1 slot riêng -> bật/tắt nhiều thiết bị
    // cùng lúc không bị ghi đè lẫn nhau.
    void registerControlCampaign(const uint8_t *payload, uint16_t len,
                                  const uint8_t *nodeMac, uint8_t deviceId);

    // Yêu cầu bắt đầu chiến dịch quét (gọi từ MQTT command handler).
    void startScan();

    // Yêu cầu bắt đầu chiến dịch xóa gateway (báo Node bỏ liên kết).
    void startDeleteGateway();

private:
};

#endif
