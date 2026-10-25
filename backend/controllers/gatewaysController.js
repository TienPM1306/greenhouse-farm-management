// gatewaysController — thêm/truy xuất/sửa/xóa Gateway (Bảng 3.4) + cổng config GĐ3c.
const Gateway = require("../models/Gateway");
const Node = require("../models/Node");
const Sensor = require("../models/Sensor");
const Device = require("../models/Device");
const Schedule = require("../models/Schedule");
const Condition = require("../models/Condition");
const Notify = require("../models/Notify");
const NewGateway = require("../models/NewGateway");
const crud = require("./crudFactory")(Gateway);
const { encodeConfig } = require("../lib/encodeConfig");
const { encodeScanRequest } = require("../lib/encodeNodeDiscovery");
const { publishCommand, publishConfig } = require("../mqttHandler");

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

// ── GĐ3c: gửi lệnh ConfigMsg xuống gateway (đăng ký/ping/hủy đăng ký) ──
// POST /v1/gateways/:id/config   body: { action: "ping" | "register" | "reset" }
//
// CỔNG ràng buộc theo trạng thái (registered):
//   - register: chỉ khi CHƯA đăng ký (đã đăng ký → 409, gửi lại vô nghĩa)
//   - reset   : chỉ khi ĐÃ đăng ký   (chưa đăng ký → 409, không có gì để hủy)
//   - ping    : luôn cho (chỉ buzzer xác nhận board)
async function configGateway(req, res, next) {
  try {
    const action = String((req.body && req.body.action) || "").toLowerCase();

    const gw = await Gateway.findById(req.params.id);
    if (!gw) return res.status(404).json({ message: "Không tìm thấy gateway" });
    if (!gw.mac) return res.status(409).json({ message: "Gateway chưa có MAC phần cứng" });

    // Cổng trạng thái
    if (action === "register" && gw.registered === true) {
      return res.status(409).json({ message: "Gateway đã đăng ký rồi", registered: true });
    }
    if (action === "reset" && gw.registered !== true) {
      return res.status(409).json({ message: "Gateway chưa đăng ký — không có gì để hủy", registered: false });
    }

    // Encode + publish
    let payloadHex;
    try {
      payloadHex = encodeConfig(action);
    } catch (e) {
      return res.status(400).json({ message: e.message });
    }
    const topic = await publishConfig(gw.mac, payloadHex);

    // register thành công → đánh dấu registered=true (firmware cũng lưu NVS phía nó).
    // reset → đặt lại false (firmware sẽ clear NVS + restart).
    if (action === "register") {
      gw.registered = true;
      await gw.save();
    } else if (action === "reset") {
      gw.registered = false;
      await gw.save();
    }

    res.json({ message: `Đã gửi lệnh ${action}`, gwMac: gw.mac, action, topic, payloadHex });
  } catch (err) {
    next(err);
  }
}

async function scanNodes(req, res, next) {
  try {
    const gw = await Gateway.findById(req.params.id);
    if (!gw) return res.status(404).json({ message: "Không tìm thấy gateway" });
    if (!gw.mac) return res.status(409).json({ message: "Gateway chưa có MAC phần cứng" });
    if (gw.registered !== true) {
      return res.status(409).json({ message: "Gateway chưa đăng ký — hãy cấu hình gateway trước" });
    }

    const configHex = encodeConfig("register");
    const configTopic = await publishConfig(gw.mac, configHex);
    await sleep(500);

    const payloadHex = encodeScanRequest();
    const topic = await publishCommand(gw.mac, payloadHex);
    res.json({
      message: "Đã gửi lệnh quét nhà kính",
      gwMac: gw.mac,
      configTopic,
      configHex,
      topic,
      payloadHex,
    });
  } catch (err) {
    next(err);
  }
}

// DELETE /v1/gateways/:id — xóa gateway VÀ cascade node/sensor/device thuộc nó.
// Khi "Cấu hình lại"/xóa dự án: node thuộc gateway phải được xóa để khi phần cứng tiếp tục
// phát node_data, mqttHandler auto-tạo lại node SẠCH (name rỗng, chưa adopt) → cấu hình từ đầu.
async function removeGateway(req, res, next) {
  try {
    const id = req.params.id;
    const gw = await Gateway.findById(id);
    if (!gw) return res.status(404).json({ message: "Không tìm thấy gateway" });

    // Tìm node thuộc gateway (theo gatewayId) để xóa kèm sensor/device của chúng.
    const nodes = await Node.find({ gatewayId: id }).select("_id").lean();
    const nodeIds = nodes.map((n) => n._id);
    let ds = { deletedCount: 0 }, ss = { deletedCount: 0 };
    if (nodeIds.length) {
      ds = await Device.deleteMany({ nodeId: { $in: nodeIds } });
      ss = await Sensor.deleteMany({ nodeId: { $in: nodeIds } });
      await Node.deleteMany({ _id: { $in: nodeIds } });
    }
    // Phòng xa: dọn sensor/device gắn trực tiếp gatewayId (nếu có).
    await Device.deleteMany({ gatewayId: id });
    await Sensor.deleteMany({ gatewayId: id });
    await Gateway.findByIdAndDelete(id);

    res.json({
      message: "Đã xóa gateway + node/sensor/device thuộc nó",
      id,
      removed: { nodes: nodeIds.length, devices: ds.deletedCount, sensors: ss.deletedCount },
    });
  } catch (err) {
    next(err);
  }
}

async function removeGatewayV2(req, res, next) {
  try {
    const id = req.params.id;
    const gw = await Gateway.findById(id);
    if (!gw) return res.status(404).json({ message: "Khong tim thay gateway" });

    const gwMac = gw.mac ? String(gw.mac).toUpperCase() : "";
    let resetTopic = null;
    let resetPayloadHex = null;
    if (gwMac) {
      resetPayloadHex = encodeConfig("reset");
      try {
        resetTopic = await publishConfig(gwMac, resetPayloadHex);
      } catch (e) {
        return res.status(503).json({
          message: "Khong gui duoc lenh reset gateway, chua xoa du an de tranh lech trang thai",
          id,
          gwMac,
          error: e.message,
        });
      }
    }

    const nodes = await Node.find({ gatewayId: id }).select("_id").lean();
    const nodeIds = nodes.map((n) => n._id);
    const nodeIdFilter = nodeIds.length ? { $in: nodeIds } : { $in: [] };

    const [dsByNode, ssByNode, schByNode, condByNode, notiByNode] = await Promise.all([
      Device.deleteMany({ nodeId: nodeIdFilter }),
      Sensor.deleteMany({ nodeId: nodeIdFilter }),
      Schedule.deleteMany({ nodeId: nodeIdFilter }),
      Condition.deleteMany({ nodeId: nodeIdFilter }),
      Notify.deleteMany({ nodeId: nodeIdFilter }),
    ]);

    const [dsByGateway, ssByGateway, schByGateway, condByGateway, notiByGateway] = await Promise.all([
      Device.deleteMany({ gatewayId: id }),
      Sensor.deleteMany({ gatewayId: id }),
      Schedule.deleteMany({ gatewayId: id }),
      Condition.deleteMany({ gatewayId: id }),
      Notify.deleteMany({ gatewayId: id }),
    ]);

    await Node.deleteMany({ _id: nodeIdFilter });
    const ng = gwMac ? await NewGateway.deleteMany({ mac: gwMac }) : { deletedCount: 0 };
    await Gateway.findByIdAndDelete(id);

    res.json({
      message: "Da reset gateway va xoa sach du an",
      id,
      reset: gwMac ? { topic: resetTopic, payloadHex: resetPayloadHex } : null,
      removed: {
        nodes: nodeIds.length,
        devices: dsByNode.deletedCount + dsByGateway.deletedCount,
        sensors: ssByNode.deletedCount + ssByGateway.deletedCount,
        schedules: schByNode.deletedCount + schByGateway.deletedCount,
        conditions: condByNode.deletedCount + condByGateway.deletedCount,
        notifies: notiByNode.deletedCount + notiByGateway.deletedCount,
        newGateways: ng.deletedCount,
      },
    });
  } catch (err) {
    next(err);
  }
}

module.exports = { ...crud, remove: removeGatewayV2, configGateway, scanNodes };
