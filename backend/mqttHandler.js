// ─────────────────────────────────────────────────────────────────────────────
// mqttHandler.js — GIAI ĐOẠN 3b. Backend nhận MQTT từ Gateway → ghi MongoDB.
// CHỈ GHI DB (subscribe + lưu). KHÔNG publish lệnh điều khiển xuống phần cứng (đó là 3c).
//
// Auto-provision theo đúng logic đăng ký:
//   - node_data  → tạo gateway (đã đăng ký) + node (CHƯA ĐẶT TÊN) + sensors nếu chưa có, rồi
//                  append điểm {time,value} vào sensor theo sensorType.
//   - newgateway → upsert vào newGateways (gateway CHƯA đăng ký).
//   - node_response → cập nhật Device.status (best-effort, bỏ qua nếu chưa có device).
//   - online     → cập nhật lastSeen của gateway.
// ─────────────────────────────────────────────────────────────────────────────
const mqtt = require("mqtt");

const Gateway = require("./models/Gateway");
const Node = require("./models/Node");
const Sensor = require("./models/Sensor");
const Device = require("./models/Device");
const NewGateway = require("./models/NewGateway");
const { broadcast } = require("./sseHub");
const Notify = require("./models/Notify");
const { evaluate } = require("./lib/alertThresholds");

// field JSON → sensorType (1=nhiệt độ, 2=độ ẩm, 3=ánh sáng)
const FIELD_TO_TYPE = { temperature: 1, humidity: 2, light: 3 };

const TOPICS = ["+/node_data", "+/node_response", "+/online", "agriculture/abc123@/newgateway"];

const log = (...a) => console.log("[MQTT]", ...a);

// Pending state = tạm thời trong bộ nhớ (không persist Mongo). Sống tối đa ~16s: từ lúc
// controlController gọi setPending() tới khi handleNodeResponse() nhận ACK hoặc timeout từ
// firmware rồi tự clearPending(). Không cần dọn rác chủ động (xem spec mục 3.2).
const pendingCommands = new Map(); // deviceId (string) -> { since: number }

// Chống race ACK vs timeout: firmware Gateway publish CẢ ACK thật (state 0/1) LẪN timeout
// (state -1) cho cùng 1 lệnh, và thứ tự tới KHÔNG cố định (đo thực tế: ACK hay tới ~cùng lúc
// hoặc SAU timeout vài trăm ms vì Node chỉ bắt được gói lệnh ở lần resend cuối). ACK là sự
// thật cao hơn timeout (ACK = Node THẬT SỰ nhận lệnh). Nên khi timeout tới, KHÔNG phát ngay
// mà hẹn ACK_WIN_MS: nếu trong lúc chờ có ACK tới → hủy timeout (ACK thắng); hết giờ không ACK
// → mới phát timeout. Ngược lại ACK tới trước thì hủy luôn timeout đang chờ (nếu có).
const pendingTimeouts = new Map(); // deviceId (string) -> setTimeout handle của timeout đang hoãn
const ACK_WIN_MS = 4000;

// ── Phần A: cảnh báo vượt ngưỡng. State edge-trigger theo node+loại cảm biến. ──
const alertState = new Map(); // "<nodeId>:<sensorType>" -> "normal"|"low"|"high"

// Phần thuần (test được, không đụng DB): quyết định có báo không dựa trạng thái trước.
function _checkThresholdPure(prevState, sensorType, value) {
  const res = evaluate(sensorType, value);
  const cur = res.kind || "normal";
  const prev = prevState || "normal";
  if (cur === "normal" || cur === prev) {
    return { shouldNotify: false, newState: cur, notifyPayload: null };
  }
  const dir = cur === "low"
    ? `< ${res.min}${res.unit}`
    : `> ${res.max}${res.unit}`;
  // Không có cận trên (max=null, vd ánh sáng) → hiển thị "≥ min" thay vì "min–∞" cho dễ hiểu.
  const safe = res.max == null
    ? `≥ ${res.min}${res.unit}`
    : `${res.min}–${res.max}${res.unit}`;
  return {
    shouldNotify: true,
    newState: cur,
    notifyPayload: {
      type: "warning",
      title: `${res.label} vượt ngưỡng`,
      message: `${value}${res.unit} ${dir} (an toàn ${safe})`,
    },
  };
}

// Giữ tối đa NOTIFY_KEEP_PER_NODE cảnh báo mới nhất mỗi node → tự dọn, không phình DB.
// Không dùng TTL/cron: dọn ngay sau khi tạo (đơn giản, đúng lúc).
const NOTIFY_KEEP_PER_NODE = 50;

async function pruneNotifies(nodeId) {
  try {
    // Lấy các notify VƯỢT quá N mới nhất (bỏ qua N đầu khi sắp xếp mới→cũ) rồi xóa.
    const stale = await Notify.find({ nodeId })
      .sort({ createdAt: -1 })
      .skip(NOTIFY_KEEP_PER_NODE)
      .select("_id");
    if (stale.length) {
      await Notify.deleteMany({ _id: { $in: stale.map((d) => d._id) } });
    }
  } catch (e) {
    log("✗ pruneNotifies lỗi:", e.message);
  }
}

// Ghép phần thuần + tạo Notify + cập nhật state. Lỗi chỉ log, KHÔNG ném ra ngoài.
async function checkThreshold(node, gw, sensorType, value) {
  try {
    const key = `${node._id}:${sensorType}`;
    const r = _checkThresholdPure(alertState.get(key), sensorType, value);
    alertState.set(key, r.newState);
    if (!r.shouldNotify) return;
    const nodeLabel = node.name && node.name.trim() ? node.name : node.mac;
    const notify = await Notify.create({
      gatewayId: gw._id,
      nodeId: node._id,
      type: r.notifyPayload.type,
      title: r.notifyPayload.title,
      message: `Node ${nodeLabel}: ${r.notifyPayload.message}`,
      createdAt: new Date().toISOString(),
    });
    // Đẩy realtime lên trang Cảnh báo (tái dùng SSE). toJSON map _id→id giống REST /v1.
    broadcast({ newNotify: notify.toJSON() });
    await pruneNotifies(node._id); // giữ tối đa 50 mới nhất/node
    log(`⚠ Notify: ${r.notifyPayload.title} — node ${nodeLabel} = ${value}`);
  } catch (e) {
    log("✗ checkThreshold lỗi:", e.message);
  }
}

// ── Phần B: cảm biến auto điều khiển thiết bị. Đèn (pin "1") theo ánh sáng, Quạt (pin "3")
//    theo nhiệt độ. Chỉ khi thiết bị mode="auto". Retry tới khi ACK về (chống mất gói). ──
const LIGHT_DARK = 3; // ánh sáng < 3 = tối → bật đèn; >= 3 = sáng → tắt
const FAN_HOT = 35;   // nhiệt > 35 = nóng → bật quạt; <= 35 → tắt (khớp buzzer firmware + cảnh báo)

// Phần thuần (dùng chung đèn + quạt): quyết định gửi lệnh theo trạng thái THẬT + pending.
// shouldBeOn do bên gọi tính (đèn: value<LIGHT_DARK; quạt: value>FAN_HOT). Gửi khi thiết bị
// CHƯA đúng trạng thái VÀ không đang chờ ACK → retry mỗi gói node_data tới khi ACK về.
function _autoDeviceAction(shouldBeOn, currentStatus, isPending) {
  if (isPending) return { send: false, on: shouldBeOn };       // đang chờ ACK → không chồng lệnh
  if (!!currentStatus === shouldBeOn) return { send: false, on: shouldBeOn }; // đã đúng → dừng
  return { send: true, on: shouldBeOn };
}

// Auto điều khiển 1 thiết bị relay theo cảm biến. Lỗi chỉ log. pin: chân relay; shouldBeOn:
// trạng thái mong muốn; nhãn để log. Retry tới khi ACK về (dựa dev.status + pending).
async function autoControlDevice(node, pin, shouldBeOn, label, sensorInfo) {
  try {
    const dev = await Device.findOne({ nodeId: node._id, pin: String(pin) });
    if (!dev || dev.mode !== "auto") return;

    const devId = dev._id.toString();
    const act = _autoDeviceAction(shouldBeOn, dev.status, pendingCommands.has(devId));
    if (!act.send) return;

    // Lazy require tránh circular: controlController require mqttHandler.
    const { controlDeviceCore } = require("./controllers/controlController");
    await controlDeviceCore(node._id, Number(pin), act.on ? "on" : "off");
    setPending(devId);
    broadcast({ deviceId: dev._id, state: "pending", status: act.on, source: "sensor" });
    log(`⚙ Auto ${label} node ${node.mac}: ${act.on ? "BẬT" : "TẮT"} (${sensorInfo}) — retry tới khi ACK`);
  } catch (e) {
    log(`✗ autoControl ${label} lỗi:`, e.message);
  }
}

// Đèn theo ánh sáng (tối → bật).
async function autoControlLight(node, value) {
  await autoControlDevice(node, 1, value < LIGHT_DARK, "đèn", `ánh sáng=${value}`);
}

// Quạt theo nhiệt độ (nóng > 40 → bật).
async function autoControlFan(node, value) {
  await autoControlDevice(node, 3, value > FAN_HOT, "quạt", `nhiệt=${value}`);
}

function setPending(deviceId) {
  pendingCommands.set(deviceId, { since: Date.now() });
}
function clearPending(deviceId) {
  pendingCommands.delete(deviceId);
}

// Client MQTT dùng chung (vừa subscribe dữ liệu, vừa publish lệnh điều khiển ở GĐ3c).
let mqttClient = null;

// Publish 1 gói lệnh (hex string) xuống <gwMac>/commands. Trả Promise.
// Dùng bởi cổng điều khiển (controlController) SAU khi đã kiểm tra node đã đăng ký.
function publishCommand(gwMac, payloadHex) {
  return new Promise((resolve, reject) => {
    if (!mqttClient || !mqttClient.connected) {
      return reject(new Error("MQTT chưa kết nối — không gửi được lệnh"));
    }
    const topic = `${String(gwMac).toUpperCase()}/commands`;
    mqttClient.publish(topic, payloadHex, { qos: 1, retain: false }, (err) => {
      if (err) return reject(err);
      log(`→ Publish lệnh: ${topic} = ${payloadHex}`);
      resolve(topic);
    });
  });
}

// Publish 1 gói ConfigMsg (hex) xuống <gwMac>/config — dùng cho đăng ký/ping/reset gateway.
function publishConfig(gwMac, payloadHex) {
  return new Promise((resolve, reject) => {
    if (!mqttClient || !mqttClient.connected) {
      return reject(new Error("MQTT chưa kết nối — không gửi được lệnh config"));
    }
    const topic = `${String(gwMac).toUpperCase()}/config`;
    mqttClient.publish(topic, payloadHex, { qos: 1, retain: false }, (err) => {
      if (err) return reject(err);
      log(`→ Publish config: ${topic} = ${payloadHex}`);
      resolve(topic);
    });
  });
}

// ─── Append 1 điểm vào sensor.data (downsample: 1 điểm/phút + 1 điểm/giờ, cuộn ngày) ───
function appendPoint(sensor, value) {
  const now = new Date();
  const iso = now.toISOString();
  const day = iso.slice(0, 10); // YYYY-MM-DD
  const minute = iso.slice(0, 16); // YYYY-MM-DDTHH:MM
  const hour = iso.slice(0, 13); // YYYY-MM-DDTHH

  if (!sensor.data || typeof sensor.data !== "object") sensor.data = {};
  if (!sensor.data.today) sensor.data.today = { dataMinute: [], dataHour: [] };
  if (!Array.isArray(sensor.data.pastDay)) sensor.data.pastDay = [];
  const t = sensor.data.today;

  // Cuộn ngày: nếu dữ liệu today thuộc ngày khác hôm nay → archive sang pastDay
  const lastMin = t.dataMinute[t.dataMinute.length - 1];
  const dataDay = lastMin ? lastMin.time.slice(0, 10) : day;
  if (lastMin && dataDay !== day) {
    sensor.data.pastDay.push({ date: dataDay, dataMinute: t.dataMinute, dataHour: t.dataHour });
    if (sensor.data.pastDay.length > 30) sensor.data.pastDay = sensor.data.pastDay.slice(-30);
    sensor.data.today = { dataMinute: [], dataHour: [] };
  }

  const today = sensor.data.today;
  // 1 điểm/phút
  const lm = today.dataMinute[today.dataMinute.length - 1];
  if (!lm || lm.time.slice(0, 16) !== minute) {
    today.dataMinute.push({ time: iso, value });
  } else {
    lm.value = value; // cùng phút → cập nhật giá trị mới nhất
  }
  // 1 điểm/giờ
  const lh = today.dataHour[today.dataHour.length - 1];
  if (!lh || lh.time.slice(0, 13) !== hour) {
    today.dataHour.push({ time: iso, value });
  } else {
    lh.value = value;
  }

  sensor.markModified("data"); // data là Mixed → bắt buộc markModified
}

// ─── node_data: {nodeMac, light, temperature, humidity} ───
async function handleNodeData(gwMac, json) {
  if (!json || !json.nodeMac) return;
  const iso = new Date().toISOString();
  const day = iso.slice(0, 10);

  // 1) Gateway (đang publish ⇒ đã đăng ký) — tìm theo mac.
  //    Nếu có NHIỀU gateway cùng mac, ưu tiên cái đã gắn DỰ ÁN (có userId/projectName) để node
  //    gắn đúng dự án của web, tránh dính vào gateway rỗng do auto-tạo lúc trước.
  const gws = await Gateway.find({ mac: gwMac });
  let gw = gws.find((g) => g.userId || g.projectName) || gws[0];
  if (!gw) {
    gw = await Gateway.create({ mac: gwMac, registered: true, gatewayCreatedDay: day, lastSeen: iso });
    log(`+ Gateway mới (đã đăng ký): ${gwMac} → id=${gw._id}`);
  } else if (!gw.registered) {
    gw.registered = true;
    gw.lastSeen = iso;
    await gw.save();
  }

  // 2) Node — upsert theo mac, name RỖNG (đặt tên sau qua web). Đánh dấu online + lastSeen.
  let node = await Node.findOne({ mac: json.nodeMac });
  if (!node) {
    node = await Node.create({
      mac: json.nodeMac,
      name: "",
      gatewayId: gw._id,
      nodeCreatedDay: day,
      online: true,
      lastSeen: iso,
    });
    log(`+ Node mới (CHƯA ĐẶT TÊN, ONLINE): ${json.nodeMac} → id=${node._id}`);
  } else {
    await Node.updateOne({ _id: node._id }, { online: true, lastSeen: iso });
  }

  // 3) Sensors theo từng đại lượng
  for (const [field, sensorType] of Object.entries(FIELD_TO_TYPE)) {
    const v = json[field];
    if (typeof v !== "number") continue;
    let sensor = await Sensor.findOne({ nodeId: node._id, sensorType });
    if (!sensor) {
      sensor = await Sensor.create({
        nodeId: node._id,
        gatewayId: gw._id,
        sensorType,
        data: { today: { dataMinute: [], dataHour: [] }, pastDay: [] },
      });
      log(`  + Sensor mới: node=${json.nodeMac} type=${sensorType}`);
    }
    appendPoint(sensor, v);
    await sensor.save();
    await checkThreshold(node, gw, sensorType, v);
    if (sensorType === 3) await autoControlLight(node, v);
    if (sensorType === 1) await autoControlFan(node, v);
  }
}

// ─── 1 node trong kết quả scan: {nodeMac, gatewayMac|null} ───
async function upsertScannedNode(gwMac, entry) {
  if (!entry || !entry.nodeMac) return;

  const nodeMac = String(entry.nodeMac).toUpperCase();
  const currentGwMac = String(gwMac).toUpperCase();
  const responseGwMac = entry.gatewayMac ? String(entry.gatewayMac).toUpperCase() : null;

  if (responseGwMac && responseGwMac !== currentGwMac) {
    log(`~ ScanResponse bỏ qua node=${nodeMac}: đang gắn gateway khác ${responseGwMac}`);
    return;
  }

  const gws = await Gateway.find({ mac: currentGwMac });
  const gw = gws.find((g) => g.userId || g.projectName) || gws[0];
  if (!gw) {
    log(`~ ScanResponse bỏ qua node=${nodeMac}: chưa có gateway mac=${currentGwMac} trong DB`);
    return;
  }

  const iso = new Date().toISOString();
  const day = iso.slice(0, 10);
  const existing = await Node.findOne({ mac: nodeMac });

  if (!existing) {
    await Node.create({
      mac: nodeMac,
      name: "",
      registered: false,
      gatewayId: gw._id,
      nodeCreatedDay: day,
      online: true,
      lastSeen: iso,
    });
    log(`+ Node scan mới (CHƯA ĐẶT TÊN): ${nodeMac} → gateway=${gw._id}`);
    return;
  }

  if (!existing.registered && !(existing.name && existing.name.trim())) {
    await Node.updateOne(
      { _id: existing._id },
      { gatewayId: gw._id, online: true, lastSeen: iso }
    );
    log(`~ Node scan cập nhật: ${nodeMac} → gateway=${gw._id}`);
  }
}

// ─── node_response cmd=13 (scan): {cmd, nodes:[{nodeMac, gatewayMac|null}, ...]} ───
// Gateway gom hết kết quả 1 campaign rồi publish 1 message duy nhất chứa toàn bộ node.
async function handleScanResponse(gwMac, json) {
  const nodes = Array.isArray(json && json.nodes) ? json.nodes : [];
  for (const entry of nodes) {
    await upsertScannedNode(gwMac, entry);
  }
}

async function handleNodeResponse(gwMac, json) {
  if (!json) return;
  if (Number(json.cmd) === 13) {
    await handleScanResponse(gwMac, json);
    return;
  }
  if (!json.nodeMac) return;

  const node = await Node.findOne({ mac: json.nodeMac });
  if (!node) return;
  const dev = await Device.findOne({ nodeId: node._id, pin: String(json.deviceId) });
  if (!dev) return;

  const deviceIdStr = dev._id.toString();

  if (Number(json.state) === -1) {
    // Timeout: Gateway hết gói không nhận ACK. KHÔNG phát ngay — hoãn ACK_WIN_MS chờ xem ACK
    // (sự thật cao hơn) có tới không. Chỉ đặt hẹn 1 lần; nếu đã có timeout đang hoãn thì bỏ qua.
    if (pendingTimeouts.has(deviceIdStr)) return;
    const handle = setTimeout(async () => {
      pendingTimeouts.delete(deviceIdStr);
      clearPending(deviceIdStr);
      const cur = await Device.findById(dev._id); // đọc lại status mới nhất
      const status = cur ? cur.status : dev.status;
      log(`~ Device ${dev._id} timeout (không nhận ACK sau ${ACK_WIN_MS}ms chờ) — giữ status=${status}`);
      broadcast({ deviceId: deviceIdStr, state: "timeout", status });
    }, ACK_WIN_MS);
    pendingTimeouts.set(deviceIdStr, handle);
    return;
  }

  // ACK thật (state 0/1): thắng mọi timeout đang hoãn cho thiết bị này.
  const pt = pendingTimeouts.get(deviceIdStr);
  if (pt) {
    clearTimeout(pt);
    pendingTimeouts.delete(deviceIdStr);
  }
  clearPending(deviceIdStr);
  dev.status = json.state === 1;
  await dev.save();
  log(`~ Device ${dev._id} state=${dev.status}`);
  broadcast({ deviceId: deviceIdStr, state: dev.status ? "on" : "off", status: dev.status });
}

// ─── newgateway (nhị phân): [01][01][6B mac][02][01][32B ssid] ───
async function handleNewGateway(buf) {
  if (buf.length < 8) return;
  const mac = buf.slice(2, 8).toString("hex").toUpperCase();
  let ssid = "";
  if (buf.length >= 10) ssid = buf.slice(10).toString("ascii").replace(/ +$/, "").trim();
  await NewGateway.findOneAndUpdate(
    { mac },
    { mac, ssid, createdAt: new Date().toISOString() },
    { upsert: true, new: true }
  );
  log(`+ NewGateway (CHƯA đăng ký): mac=${mac} ssid=${ssid}`);
}

// ─── Quét node offline (on/off logic) ───
// Node coi là OFFLINE nếu quá OFFLINE_MS không có node_data.
//   - Chưa đăng ký (registered=false VÀ name rỗng) + offline → XÓA node + sensors + devices.
//   - Đã đăng ký (registered=true HOẶC đã đặt tên)        + offline → chỉ set online=false (giữ lại).
const OFFLINE_MS = 60000; // Giu node vua scan du lau de user kip dat ten/adopt.
const CHECK_MS = 5000; // chu kỳ quét

async function sweepOffline() {
  try {
    const cutoffMs = Date.now() - OFFLINE_MS;
    // Duyệt tất cả node (số lượng nhỏ); quyết theo lastSeen (thiếu lastSeen = coi như offline).
    const nodes = await Node.find();
    for (const node of nodes) {
      const lastMs = node.lastSeen ? Date.parse(node.lastSeen) : 0;
      const offline = !(lastMs >= cutoffMs); // chưa từng / quá lâu không có dữ liệu
      if (!offline) continue;

      const adopted = node.registered || (node.name && node.name.trim() !== "");
      if (adopted) {
        if (node.online !== false) {
          await Node.updateOne({ _id: node._id }, { online: false });
          log(`~ Node ${node.mac} OFFLINE (đã đăng ký) → giữ lại, online=false`);
        }
      } else {
        await Sensor.deleteMany({ nodeId: node._id });
        await Device.deleteMany({ nodeId: node._id });
        await Node.deleteOne({ _id: node._id });
        log(`- Node ${node.mac || node._id} OFFLINE (chưa đăng ký) → đã XÓA node + sensors`);
      }
    }

    // Dọn sensor/device MỒ CÔI (nodeId không trỏ tới node nào còn tồn tại) — tự lành nếu
    // có node bị xóa dở (vd process bị kill giữa chừng) để không tích tụ rác.
    const ids = (await Node.find().select("_id").lean()).map((n) => n._id);
    const rs = await Sensor.deleteMany({ nodeId: { $nin: ids } });
    const rd = await Device.deleteMany({ nodeId: { $nin: ids } });
    if (rs.deletedCount || rd.deletedCount) {
      log(`- Dọn mồ côi: ${rs.deletedCount} sensors, ${rd.deletedCount} devices`);
    }
  } catch (e) {
    log("✗ sweepOffline lỗi:", e.message);
  }
}

// ─── Khởi động: kết nối + subscribe ───
function startMqtt() {
  const { MQTT_HOST, MQTT_USERNAME, MQTT_PASSWORD } = process.env;
  const PORT = Number(process.env.MQTT_PORT || 8883);
  if (!MQTT_HOST || !MQTT_USERNAME) {
    log("⚠ Bỏ qua MQTT: thiếu MQTT_HOST/MQTT_USERNAME trong .env (REST vẫn chạy).");
    return null;
  }

  const client = mqtt.connect(`mqtts://${MQTT_HOST}:${PORT}`, {
    username: MQTT_USERNAME,
    password: MQTT_PASSWORD,
    reconnectPeriod: 5000,
  });
  mqttClient = client; // lưu để publishCommand dùng lại

  client.on("connect", () => {
    log("✓ Kết nối broker:", MQTT_HOST);
    client.subscribe(TOPICS, (err) => {
      if (err) return log("✗ Subscribe lỗi:", err.message);
      log("✓ Subscribed:", TOPICS.join(", "));
    });
  });

  client.on("message", async (topic, buf) => {
    try {
      const gwMac = topic.split("/")[0];
      const kind = topic.split("/").slice(1).join("/");
      if (kind === "node_data") {
        await handleNodeData(gwMac, JSON.parse(buf.toString()));
      } else if (kind === "node_response") {
        await handleNodeResponse(gwMac, JSON.parse(buf.toString()));
      } else if (kind === "online") {
        await Gateway.updateOne({ mac: gwMac }, { lastSeen: new Date().toISOString() });
      } else if (topic.endsWith("/newgateway")) {
        await handleNewGateway(buf);
      }
    } catch (e) {
      log("✗ Lỗi xử lý message", topic, ":", e.message);
    }
  });

  client.on("error", (err) => log("✗ Lỗi MQTT:", err.message));

  // Bật vòng quét node offline
  setInterval(sweepOffline, CHECK_MS);
  log(`✓ Bật quét offline: ngưỡng ${OFFLINE_MS / 1000}s, chu kỳ ${CHECK_MS / 1000}s`);

  return client;
}

// Trạng thái kết nối MQTT của backend (cho health-check). true = đang nối broker.
function isMqttConnected() {
  return !!(mqttClient && mqttClient.connected);
}

module.exports = {
  startMqtt,
  sweepOffline,
  OFFLINE_MS,
  publishCommand,
  publishConfig,
  isMqttConnected,
  setPending,
  clearPending,
  _checkThresholdPure,
  _autoDeviceAction,
  LIGHT_DARK,
  FAN_HOT,
};
