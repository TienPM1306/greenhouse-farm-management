// ─────────────────────────────────────────────────────────────────────────────
// scheduler.js — Engine thực thi "Lịch hẹn giờ". Quét mỗi 60s: lịch nào đang
// status:true VÀ trong khung giờ -> thiết bị đó phải BẬT; ngược lại phải TẮT.
// So với Device.status thực tế, lệch mới gửi lệnh (tránh spam khi đã đúng).
// Lỗi gửi lệnh (node chưa đăng ký, gateway mất MAC...) chỉ log, KHÔNG crash
// interval -> tự thử lại ở lượt quét sau (bù trễ/rớt gói LoRa tự nhiên).
// ─────────────────────────────────────────────────────────────────────────────
const Schedule = require("./models/Schedule");
const Device = require("./models/Device");
const { controlDeviceCore } = require("./controllers/controlController");
const { isActiveNow } = require("./lib/scheduleTime");
const Sensor = require("./models/Sensor");
const { setPending, LIGHT_DARK, FAN_HOT } = require("./mqttHandler");
const { broadcast } = require("./sseHub");

const CHECK_INTERVAL_MS = 15000;

const log = (...a) => console.log("[Scheduler]", ...a);
let ticking = false;
let rerunRequested = false;

// Đọc giá trị ánh sáng mới nhất của node (sensor type=3, điểm cuối dataMinute). null nếu không có.
async function latestLight(nodeId) {
  const s = await Sensor.findOne({ nodeId, sensorType: 3 });
  const arr = s && s.data && s.data.today && s.data.today.dataMinute;
  if (!Array.isArray(arr) || !arr.length) return null;
  const last = arr[arr.length - 1];
  return typeof last.value === "number" ? last.value : null;
}

// Đọc nhiệt độ mới nhất của node (sensor type=1, điểm cuối dataMinute). null nếu không có.
async function latestTemp(nodeId) {
  const s = await Sensor.findOne({ nodeId, sensorType: 1 });
  const arr = s && s.data && s.data.today && s.data.today.dataMinute;
  if (!Array.isArray(arr) || !arr.length) return null;
  const last = arr[arr.length - 1];
  return typeof last.value === "number" ? last.value : null;
}

async function applyDeviceState(deviceId, shouldBeOn, { notifyNoop = false } = {}) {
  const device = await Device.findById(deviceId);
  if (!device) return;

  // Đèn (pin "1"): chỉ scheduler tác động khi mode="auto". Manual → bỏ qua hoàn toàn.
  if (String(device.pin) === "1") {
    if (device.mode !== "auto") return;
    // Cảm biến ưu tiên hơn lịch: trời tối (<LIGHT_DARK) thì cảm biến giữ đèn bật → scheduler bỏ qua.
    const light = await latestLight(device.nodeId);
    if (light != null && light < LIGHT_DARK) {
      log(`Đèn node=${device.nodeId} trời tối (ánh sáng=${light}) — cảm biến giữ quyền, bỏ qua lịch`);
      return;
    }
  }

  // Quạt (pin "3"): chỉ scheduler tác động khi mode="auto". Manual → bỏ qua hoàn toàn.
  if (String(device.pin) === "3") {
    if (device.mode !== "auto") return;
    // Cảm biến ưu tiên hơn lịch: nóng (>FAN_HOT) thì cảm biến giữ quạt bật → scheduler bỏ qua.
    const temp = await latestTemp(device.nodeId);
    if (temp != null && temp > FAN_HOT) {
      log(`Quạt node=${device.nodeId} đang nóng (nhiệt=${temp}) — cảm biến giữ quyền, bỏ qua lịch`);
      return;
    }
  }

  if (!!device.status === shouldBeOn) {
    if (notifyNoop) {
      broadcast({
        deviceId,
        state: shouldBeOn ? "on" : "off",
        status: shouldBeOn,
        source: "schedule",
      });
    }
    return;
  }

  try {
    await controlDeviceCore(device.nodeId, Number(device.pin), shouldBeOn ? "on" : "off");
    setPending(deviceId);
    broadcast({
      deviceId,
      state: "pending",
      status: shouldBeOn,
      source: "schedule",
    });
    log(`Đã gửi lệnh ${shouldBeOn ? "ON" : "OFF"} device=${deviceId}`);
  } catch (e) {
    broadcast({
      deviceId,
      state: "error",
      status: device.status,
      desiredStatus: shouldBeOn,
      source: "schedule",
      message: e.message,
    });
    log(`Lỗi điều khiển device=${deviceId}:`, e.message);
  }
}

async function tick() {
  if (ticking) {
    rerunRequested = true;
    return;
  }
  ticking = true;
  try {
    const activeSchedules = await Schedule.find({ status: true });
    const now = new Date();

    // Gom theo deviceId: thiết bị cần BẬT nếu CÓ ÍT NHẤT 1 lịch active khớp giờ.
    const desiredOnByDevice = new Map();
    for (const sch of activeSchedules) {
      if (!sch.deviceId) continue; // lịch cũ thiếu deviceId (dữ liệu seed) -> bỏ qua
      const on = isActiveNow(sch, now);
      if (on) desiredOnByDevice.set(sch.deviceId, true);
      else if (!desiredOnByDevice.has(sch.deviceId)) desiredOnByDevice.set(sch.deviceId, false);
    }

    for (const [deviceId, shouldBeOn] of desiredOnByDevice) {
      await applyDeviceState(deviceId, shouldBeOn);
    }
  } catch (e) {
    log("Lỗi tick:", e.message);
  } finally {
    ticking = false;
    if (rerunRequested) {
      rerunRequested = false;
      setImmediate(tick);
    }
  }
}

function startScheduler() {
  setInterval(tick, CHECK_INTERVAL_MS);
  setImmediate(tick);
  log(`Bật: quét mỗi ${CHECK_INTERVAL_MS / 1000}s`);
}

module.exports = { startScheduler, tick, applyDeviceState };
