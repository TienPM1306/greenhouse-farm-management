#ifndef COMMAND_H
#define COMMAND_H

enum CommandType {
    CMD_UNKNOWN = 0,

    // Uplink: Node -> Gateway (dữ liệu định kỳ)
    CMD_SYNC_SENSOR_DATA    = 1,

    // Downlink: MQTT -> Gateway -> LoRa -> Node
    CMD_CHANGE_GATEWAY_MAC  = 2,
    CMD_CONTROL_DEVICE      = 3,
    CMD_STATE_DEVICE        = 4,
    CMD_REQUEST_ALL_STATES  = 5,
    CMD_SCAN_REQUEST        = 6,   // quét/đọc Node: Gateway broadcast yêu cầu Node báo danh tính
    CMD_CHANGE_TIME         = 7,

    // Gateway-internal (xử lý tại Gateway, không forward qua LoRa)
    CMD_WIFI_RESET          = 8,

    // Xóa Gateway: Gateway xóa trạng thái đăng ký + broadcast cho Node bỏ
    // liên kết (Node xóa GatewayMac đã lưu nếu trùng Gateway này).
    CMD_DELETE_GATEWAY      = 9,

    // Uplink: Node -> Gateway (phản hồi lệnh điều khiển)
    CMD_CONTROL_RESPONSE        = 10,
    CMD_STATE_RESPONSE          = 11,
    CMD_ALL_STATES_RESPONSE     = 12,
    CMD_SCAN_RESPONSE           = 13,  // Node trả lời quét: NodeMac (+ GatewayMac nếu đã có)
    CMD_CHANGE_TIME_RESPONSE    = 14,
    CMD_CHANGE_GATEWAY_RESPONSE = 15,
};

#endif
