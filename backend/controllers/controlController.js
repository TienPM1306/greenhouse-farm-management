// ─────────────────────────────────────────────────────────────────────────────
// controlController.js — GĐ3c: CỔNG điều khiển thiết bị (relay) qua backend.
// POST /v1/nodes/:id/control   body: { deviceId: 1|2|3, action: "on"|"off" }
//
// CỔNG ĐĂNG KÝ: chỉ node ĐÃ ĐĂNG KÝ (registered === true) mới được điều khiển.
//   - Node chưa đăng ký  → 403, KHÔNG publish lệnh (bị chặn ngay tại backend).
//   - Node đã đăng ký    → encode CMD_CONTROL_DEVICE → publish <gwMAC>/commands → 200.
// (Firmware luôn "mở"; cổng này nằm ở backend nên chỉ có hiệu lực khi điều khiển ĐI QUA API.)
//
// controlDeviceCore() tách riêng phần lõi (không phụ thuộc req/res) để scheduler.js
// (lịch hẹn giờ) tái sử dụng đúng 1 đường gửi lệnh, không lặp code.
// ─────────────────────────────────────────────────────────────────────────────
const Node = require("../models/Node");
const Gateway = require("../models/Gateway");
const Device = require("../models/Device");
const { encodeControl, DEVICE_NAME } = require("../lib/encodeControl");
const { publishCommand, setPending } = require("../mqttHandler");

// Ném Error với .message mô tả lý do nếu node/gateway không hợp lệ hoặc encode lỗi.
// Trả về MQTT topic đã publish nếu thành công.
async function controlDeviceCore(nodeId, deviceId, action) {
  const node = await Node.findById(nodeId);
  if (!node) throw new Error("Không tìm thấy node");
  if (node.registered !== true) throw new Error("Node chưa đăng ký");
  if (!node.mac) throw new Error("Node chưa có MAC phần cứng");

  const gw = node.gatewayId ? await Gateway.findById(node.gatewayId) : null;
  if (!gw || !gw.mac) throw new Error("Không xác định được gateway/MAC để gửi lệnh");

  const payloadHex = encodeControl(node.mac, deviceId, action);
  return publishCommand(gw.mac, payloadHex);
}

async function controlNode(req, res, next) {
  try {
    const { deviceId, action } = req.body || {};

    const node = await Node.findById(req.params.id);
    if (!node) return res.status(404).json({ message: "Không tìm thấy node" });

    if (node.registered !== true) {
      return res.status(403).json({
        message: "Node chưa đăng ký — không được phép điều khiển thiết bị",
        nodeId: node._id,
        registered: false,
      });
    }

    if (!node.mac) return res.status(409).json({ message: "Node chưa có MAC phần cứng" });
    const gw = node.gatewayId ? await Gateway.findById(node.gatewayId) : null;
    if (!gw || !gw.mac) {
      return res.status(409).json({ message: "Không xác định được gateway/MAC để gửi lệnh" });
    }

    // Chặn lệnh TAY nếu thiết bị đích đang ở chế độ Tự động (auto) — chỉ scheduler/điều
    // kiện tự động (đi qua controlDeviceCore) mới được điều khiển thiết bị này.
    const targetDev = await Device.findOne({ nodeId: node._id, pin: String(deviceId) });
    if (targetDev && targetDev.mode === "auto") {
      return res.status(403).json({
        message: "Thiết bị đang ở chế độ Tự động — không điều khiển tay được",
        deviceId: Number(deviceId),
        mode: "auto",
      });
    }

    let payloadHex;
    try {
      payloadHex = encodeControl(node.mac, deviceId, action);
    } catch (e) {
      return res.status(400).json({ message: e.message });
    }
    const topic = await publishCommand(gw.mac, payloadHex);

    const dev = await Device.findOne({ nodeId: node._id, pin: String(deviceId) });
    if (dev) setPending(dev._id.toString());

    res.json({
      message: `Đã gửi lệnh ${String(action).toUpperCase()} ${DEVICE_NAME[Number(deviceId)] || "device " + deviceId}`,
      nodeMac: node.mac,
      gwMac: gw.mac,
      deviceId: Number(deviceId),
      action,
      topic,
      payloadHex,
    });
  } catch (err) {
    next(err);
  }
}

module.exports = { controlNode, controlDeviceCore };
