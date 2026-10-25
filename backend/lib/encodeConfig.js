// ─────────────────────────────────────────────────────────────────────────────
// encodeConfig.js — Encode gói ConfigMsg (TLV nhị phân) gửi xuống <gwMAC>/config.
// Khớp firmware (message.h ConfigMsg_fields):
//   field1 isConnectedServer : PROTO_BOOL (type 0), 1 byte
//   field2 pingTest          : PROTO_BOOL (type 0), 1 byte
// Mỗi field present = [field_id][type][value]. type BOOL = 0x00.
//
// 3 action:
//   ping     = pingTest=true            → "020001"  (buzzer xác nhận board, không đổi NVS)
//   register = isConnectedServer=true   → "010001"  (lưu NVS registered=true, bắt đầu node_data)
//   reset    = isConnectedServer=false  → "010000"  (clear NVS + ESP.restart())
//
// Trả về CHUỖI HEX (in hoa) — đúng định dạng publish.
// ─────────────────────────────────────────────────────────────────────────────

const CONFIG_PAYLOADS = {
  ping: "020001", // field2 pingTest, BOOL, value=1
  register: "010001", // field1 isConnectedServer, BOOL, value=1
  reset: "010000", // field1 isConnectedServer, BOOL, value=0
};

// action: "ping" | "register" | "reset" → hex string (in hoa)
function encodeConfig(action) {
  const act = String(action || "").toLowerCase();
  const hex = CONFIG_PAYLOADS[act];
  if (!hex) throw new Error('action phải là "ping" | "register" | "reset"');
  return hex.toUpperCase();
}

module.exports = { encodeConfig, CONFIG_PAYLOADS };
