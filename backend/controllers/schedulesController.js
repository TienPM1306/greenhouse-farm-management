// schedulesController — quản lý lịch hẹn giờ bật/tắt thiết bị (Bảng 3.4).
const Schedule = require("../models/Schedule");
const crud = require("./crudFactory")(Schedule);
const { tick, applyDeviceState } = require("../scheduler");
const { isActiveNow } = require("../lib/scheduleTime");

function applySchedulesSoon() {
  setImmediate(() => {
    tick().catch((err) => console.error("[Scheduler] Lỗi áp dụng lịch:", err.message));
  });
}

function desiredStateForWrite(before, after) {
  const now = new Date();
  if (after && after.status === true && isActiveNow(after, now)) {
    return { deviceId: after.deviceId, shouldBeOn: true };
  }
  if (before && before.status === true && isActiveNow(before, now)) {
    return { deviceId: before.deviceId, shouldBeOn: false };
  }
  return null;
}

function applyDeviceStateSoon(decision) {
  if (!decision || !decision.deviceId) return;
  setImmediate(() => {
    applyDeviceState(decision.deviceId, decision.shouldBeOn, { notifyNoop: true })
      .catch((err) => console.error("[Scheduler] Lỗi điều khiển theo lịch:", err.message));
  });
}

async function runAndApply(handler, req, res, next, getDecision = null) {
  const decision = getDecision ? await getDecision("before") : null;
  await handler(req, res, next);
  if (res.headersSent && res.statusCode < 400) {
    if (getDecision) {
      const finalDecision = await getDecision("after", decision);
      applyDeviceStateSoon(finalDecision);
    }
    applySchedulesSoon();
  }
}

async function create(req, res, next) {
  try {
    const body = { ...req.body };
    if (body.id && !body._id) {
      body._id = body.id;
      delete body.id;
    }
    // Đèn (pin "1") KHÔNG dùng lịch hẹn giờ — chỉ Thủ công + Tự động (cảm biến ánh sáng).
    // Chặn tại backend kể cả khi UI bị bypass. Quạt (3)/Máy bơm (2) vẫn cho tạo lịch.
    if (String(body.devicePin) === "1") {
      return res.status(400).json({
        message: "Đèn không hỗ trợ lịch hẹn giờ — chỉ điều khiển Thủ công hoặc Tự động (cảm biến)",
        devicePin: "1",
      });
    }
    const doc = await Schedule.create(body);
    res.status(201).json(doc);
    applyDeviceStateSoon(desiredStateForWrite(null, doc));
    applySchedulesSoon();
  } catch (err) {
    next(err);
  }
}

async function update(req, res, next) {
  let before = null;
  await runAndApply(crud.update, req, res, next, async (phase, capturedBefore) => {
    if (phase === "before") {
      before = await Schedule.findById(req.params.id);
      return before;
    }
    const after = await Schedule.findById(req.params.id);
    return desiredStateForWrite(capturedBefore || before, after);
  });
}

async function remove(req, res, next) {
  let before = null;
  await runAndApply(crud.remove, req, res, next, async (phase, capturedBefore) => {
    if (phase === "before") {
      before = await Schedule.findById(req.params.id);
      return before;
    }
    return desiredStateForWrite(capturedBefore || before, null);
  });
}

module.exports = {
  ...crud,
  create,
  update,
  remove,
};
