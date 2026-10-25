// ─────────────────────────────────────────────────────────────────────────────
// encodeControl.js — Encode gói CMD_CONTROL_DEVICE (TLV nhị phân) cho NodeControl.
// Khớp firmware Gateway (protoc.h / message.h / command.h): mỗi field present =
//   [field_id(1B)][type(1B)][value]. Gói lệnh = [cmd_byte][TLV...].
//
//   CMD_CONTROL_DEVICE = 0x03
//   field1 nodeMac : type UINT8 nhưng value 6 byte (MAC)
//   field2 deviceId: UINT8 (1=Đèn, 2=Máy bơm, 3=Quạt)
//   field3 action  : UINT8 (ON=1, OFF=2)
//
// Trả về CHUỖI HEX (in hoa) — đúng định dạng publish lên <gwMAC>/commands.
// ─────────────────────────────────────────────────────────────────────────────

const CMD_CONTROL_DEVICE = 0x03;
const ACTION = { on: 1, off: 2 };
const DEVICE_NAME = { 1: "Đèn", 2: "Máy bơm", 3: "Quạt" };

// nodeMac: 12 hex (vd "ECE3347B678C"); deviceId: 1|2|3; action: "on"|"off"
function encodeControl(nodeMac, deviceId, action) {
  const macHex = String(nodeMac || "").toUpperCase();
  const dev = Number(deviceId);
  const act = String(action || "").toLowerCase();

  if (!/^[0-9A-F]{12}$/.test(macHex)) throw new Error("nodeMac không hợp lệ (cần 12 hex)");
  if (![1, 2, 3].includes(dev)) throw new Error("deviceId phải là 1|2|3");
  if (!(act in ACTION)) throw new Error('action phải là "on"|"off"');

  const mac = Buffer.from(macHex, "hex"); // 6 byte
  const pkt = Buffer.concat([
    Buffer.from([CMD_CONTROL_DEVICE]),
    Buffer.from([0x01, 0x01]), mac, // field1 nodeMac (UINT8, 6B)
    Buffer.from([0x02, 0x01, dev & 0xff]), // field2 deviceId
    Buffer.from([0x03, 0x01, ACTION[act]]), // field3 action
  ]);
  return pkt.toString("hex").toUpperCase();
}

module.exports = { encodeControl, ACTION, DEVICE_NAME, CMD_CONTROL_DEVICE };
