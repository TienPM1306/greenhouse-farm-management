#ifndef PROTOBUF_MESSAGES_H
#define PROTOBUF_MESSAGES_H

#include "protoc.h"
#include <stddef.h>

#define OFFSET(type, field) ((uint16_t)(uintptr_t)&(((type *)0)->field))

// ========================= SensorData =========================

typedef struct
{
    bool    has_nodeMac;
    uint8_t nodeMac[6];

    bool    has_gatewayMac;
    uint8_t gatewayMac[6];
} SensorIdentity;

typedef struct
{
    bool  has_light;
    float light;

    bool  has_temperature;
    float temperature;

    bool  has_humidity;
    float humidity;
} SensorBlob;

typedef struct
{
    SensorIdentity identity;
    SensorBlob     data;
} SensorData;

static const ProtoField SensorData_fields[] = {
    {1, PROTO_UINT8, OFFSET(SensorData, identity.nodeMac),    6,            OFFSET(SensorData, identity.has_nodeMac)},
    {2, PROTO_UINT8, OFFSET(SensorData, identity.gatewayMac), 6,            OFFSET(SensorData, identity.has_gatewayMac)},
    {3, PROTO_FLOAT, OFFSET(SensorData, data.light),           sizeof(float), OFFSET(SensorData, data.has_light)},
    {4, PROTO_FLOAT, OFFSET(SensorData, data.temperature),     sizeof(float), OFFSET(SensorData, data.has_temperature)},
    {5, PROTO_FLOAT, OFFSET(SensorData, data.humidity),        sizeof(float), OFFSET(SensorData, data.has_humidity)},
};
#define SensorData_field_count 5

// ========================= ChangeGatewayMac =========================

typedef struct
{
    bool    has_newGatewayMac;
    uint8_t newGatewayMac[6];

    bool    has_nodeMac;
    uint8_t nodeMac[6];
} ChangeGatewayMac;

static const ProtoField ChangeGatewayMac_fields[] = {
    {1, PROTO_UINT8, OFFSET(ChangeGatewayMac, newGatewayMac), 6, OFFSET(ChangeGatewayMac, has_newGatewayMac)},
    {2, PROTO_UINT8, OFFSET(ChangeGatewayMac, nodeMac),       6, OFFSET(ChangeGatewayMac, has_nodeMac)},
};
#define ChangeGatewayMac_field_count 2

// ========================= ControlDevice =========================

typedef struct
{
    enum ControlAction_consts {
        CONTROL_ACTION_UNKNOWN = 0,
        CONTROL_ACTION_ON      = 1,
        CONTROL_ACTION_OFF     = 2
    };
    typedef uint8_t ControlAction;

    bool          has_nodeMac;
    uint8_t       nodeMac[6];

    bool          has_deviceId;
    uint8_t       deviceId;

    bool          has_action;
    ControlAction action;
} ControlDevice;

static const ProtoField ControlDevice_fields[] = {
    {1, PROTO_UINT8, OFFSET(ControlDevice, nodeMac),  6,              OFFSET(ControlDevice, has_nodeMac)},
    {2, PROTO_UINT8, OFFSET(ControlDevice, deviceId), sizeof(uint8_t), OFFSET(ControlDevice, has_deviceId)},
    {3, PROTO_UINT8, OFFSET(ControlDevice, action),   sizeof(uint8_t), OFFSET(ControlDevice, has_action)},
};
#define ControlDevice_field_count 3

// ========================= StateDevice =========================

typedef struct
{
    enum DeviceState_consts {
        DEVICE_STATE_UNKNOWN = 0,
        DEVICE_STATE_ON      = 1,
        DEVICE_STATE_OFF     = 2
    };
    typedef uint8_t DeviceState;

    bool        has_nodeMac;
    uint8_t     nodeMac[6];

    bool        has_deviceId;
    uint8_t     deviceId;

    bool        has_state;
    DeviceState state;
} StateDevice;

static const ProtoField StateDevice_fields[] = {
    {1, PROTO_UINT8, OFFSET(StateDevice, nodeMac),  6,              OFFSET(StateDevice, has_nodeMac)},
    {2, PROTO_UINT8, OFFSET(StateDevice, deviceId), sizeof(uint8_t), OFFSET(StateDevice, has_deviceId)},
    {3, PROTO_UINT8, OFFSET(StateDevice, state),    sizeof(uint8_t), OFFSET(StateDevice, has_state)},
};
#define StateDevice_field_count 3

// ========================= RequestAllStates =========================

typedef struct
{
    bool    has_nodeMac;
    uint8_t nodeMac[6];
} RequestAllStates;

static const ProtoField RequestAllStates_fields[] = {
    {1, PROTO_UINT8, OFFSET(RequestAllStates, nodeMac), 6, OFFSET(RequestAllStates, has_nodeMac)},
};
#define RequestAllStates_field_count 1

// ========================= ChangeTime =========================

typedef struct
{
    bool     has_nodeMac;
    uint8_t  nodeMac[6];

    bool     has_newTime;
    uint32_t newTime;
} ChangeTime;

static const ProtoField ChangeTime_fields[] = {
    {1, PROTO_UINT8,  OFFSET(ChangeTime, nodeMac), 6,                OFFSET(ChangeTime, has_nodeMac)},
    {2, PROTO_UINT32, OFFSET(ChangeTime, newTime),  sizeof(uint32_t), OFFSET(ChangeTime, has_newTime)},
};
#define ChangeTime_field_count 2

// ========================= ControlResponse =========================

typedef struct
{
    bool    has_nodeMac;
    uint8_t nodeMac[6];

    bool    has_deviceId;
    uint8_t deviceId;

    bool    has_state;
    uint8_t state;
} ControlResponse;

static const ProtoField ControlResponse_fields[] = {
    {1, PROTO_UINT8, OFFSET(ControlResponse, nodeMac),  6,              OFFSET(ControlResponse, has_nodeMac)},
    {2, PROTO_UINT8, OFFSET(ControlResponse, deviceId), sizeof(uint8_t), OFFSET(ControlResponse, has_deviceId)},
    {3, PROTO_UINT8, OFFSET(ControlResponse, state),    sizeof(uint8_t), OFFSET(ControlResponse, has_state)},
};
#define ControlResponse_field_count 3

// ========================= StateResponse =========================

typedef struct
{
    bool    has_nodeMac;
    uint8_t nodeMac[6];

    bool    has_deviceId;
    uint8_t deviceId;

    bool    has_state;
    uint8_t state;
} StateResponse;

static const ProtoField StateResponse_fields[] = {
    {1, PROTO_UINT8, OFFSET(StateResponse, nodeMac),  6,              OFFSET(StateResponse, has_nodeMac)},
    {2, PROTO_UINT8, OFFSET(StateResponse, deviceId), sizeof(uint8_t), OFFSET(StateResponse, has_deviceId)},
    {3, PROTO_UINT8, OFFSET(StateResponse, state),    sizeof(uint8_t), OFFSET(StateResponse, has_state)},
};
#define StateResponse_field_count 3

// ========================= AllStatesResponse =========================

typedef struct
{
    bool     has_nodeMac;
    uint8_t  nodeMac[6];

    bool     has_deviceStates;
    uint16_t deviceStates;
} AllStatesResponse;

static const ProtoField AllStatesResponse_fields[] = {
    {1, PROTO_UINT8,  OFFSET(AllStatesResponse, nodeMac),      6,                OFFSET(AllStatesResponse, has_nodeMac)},
    {2, PROTO_UINT16, OFFSET(AllStatesResponse, deviceStates), sizeof(uint16_t), OFFSET(AllStatesResponse, has_deviceStates)},
};
#define AllStatesResponse_field_count 2

// ========================= ChangeTimeResponse =========================

typedef struct
{
    bool    has_nodeMac;
    uint8_t nodeMac[6];

    bool has_success;
    bool success;
} ChangeTimeResponse;

static const ProtoField ChangeTimeResponse_fields[] = {
    {1, PROTO_UINT8, OFFSET(ChangeTimeResponse, nodeMac), 6,           OFFSET(ChangeTimeResponse, has_nodeMac)},
    {2, PROTO_BOOL,  OFFSET(ChangeTimeResponse, success), sizeof(bool), OFFSET(ChangeTimeResponse, has_success)},
};
#define ChangeTimeResponse_field_count 2

// ========================= ConfigMsg =========================

typedef struct
{
    bool has_isConnectedServer;
    bool isConnectedServer;

    bool has_pingTest;
    bool pingTest;
} ConfigMsg;

static const ProtoField ConfigMsg_fields[] = {
    {1, PROTO_BOOL, OFFSET(ConfigMsg, isConnectedServer), sizeof(bool), OFFSET(ConfigMsg, has_isConnectedServer)},
    {2, PROTO_BOOL, OFFSET(ConfigMsg, pingTest),          sizeof(bool), OFFSET(ConfigMsg, has_pingTest)},
};
#define ConfigMsg_field_count 2

// ========================= ChangeGatewayResponse =========================

typedef struct
{
    bool    has_nodeMac;
    uint8_t nodeMac[6];

    bool has_success;
    bool success;
} ChangeGatewayResponse;

static const ProtoField ChangeGatewayResponse_fields[] = {
    {1, PROTO_UINT8, OFFSET(ChangeGatewayResponse, nodeMac), 6,           OFFSET(ChangeGatewayResponse, has_nodeMac)},
    {2, PROTO_BOOL,  OFFSET(ChangeGatewayResponse, success), sizeof(bool), OFFSET(ChangeGatewayResponse, has_success)},
};
#define ChangeGatewayResponse_field_count 2

// ========================= DeleteGateway =========================
// Gateway broadcast yêu cầu Node bỏ liên kết. Node chỉ xóa GatewayMac đã lưu
// nếu trùng gatewayMac này (không đụng Node của Gateway khác).

typedef struct
{
    bool    has_gatewayMac;
    uint8_t gatewayMac[6];
} DeleteGateway;

static const ProtoField DeleteGateway_fields[] = {
    {1, PROTO_UINT8, OFFSET(DeleteGateway, gatewayMac), 6, OFFSET(DeleteGateway, has_gatewayMac)},
};
#define DeleteGateway_field_count 1

// ========================= ScanRequest =========================
// Gateway broadcast quét Node. sessionId định danh 1 đợt quét: Gateway phát
// lặp cùng sessionId trong 10s; Node chỉ đáp 1 lần cho mỗi sessionId mới.

typedef struct
{
    bool    has_sessionId;
    uint8_t sessionId;
} ScanRequest;

static const ProtoField ScanRequest_fields[] = {
    {1, PROTO_UINT8, OFFSET(ScanRequest, sessionId), sizeof(uint8_t), OFFSET(ScanRequest, has_sessionId)},
};
#define ScanRequest_field_count 1

// ========================= ScanResponse =========================
// Node trả lời lệnh quét (CMD_SCAN_REQUEST).
// nodeMac LUÔN có. gatewayMac chỉ có khi Node đã lưu Gateway MAC (NVS);
// Node chưa gắn Gateway thì bỏ trống gatewayMac nhưng vẫn gửi nodeMac.

typedef struct
{
    bool    has_nodeMac;
    uint8_t nodeMac[6];

    bool    has_gatewayMac;
    uint8_t gatewayMac[6];
} ScanResponse;

static const ProtoField ScanResponse_fields[] = {
    {1, PROTO_UINT8, OFFSET(ScanResponse, nodeMac),    6, OFFSET(ScanResponse, has_nodeMac)},
    {2, PROTO_UINT8, OFFSET(ScanResponse, gatewayMac), 6, OFFSET(ScanResponse, has_gatewayMac)},
};
#define ScanResponse_field_count 2

#endif
