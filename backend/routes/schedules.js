const router = require("express").Router();
const c = require("../controllers/schedulesController");
const Schedule = require("../models/Schedule");
const { rangesOverlap, fmtRange } = require("../lib/scheduleTime");

// Chặn khi khung giờ giao với lịch khác đang status:true cùng deviceId.
// Áp dụng ở 2 nhánh: (a) PATCH bật switch (status:true), (b) PATCH đổi giờ
// trên lịch đang active mà không kèm status — giờ mới có thể đụng lịch khác.
async function checkOverlap(req, res, next) {
  try {
    const wantsActive = req.body.status === true;
    // Chạy check khi: (a) đang bật switch (status:true), HOẶC
    // (b) đang đổi giờ trên lịch đang active (không kèm status → không tắt,
    //     nhưng giờ mới có thể đụng lịch khác đang bật).
    const isTimeChange = req.params.id && (req.body.startTime || req.body.endTime) && req.body.status === undefined;

    let deviceId = req.body.deviceId;
    let candidate = { startTime: req.body.startTime, endTime: req.body.endTime };
    let shouldCheck = wantsActive;

    if (req.params.id) {
      const existing = await Schedule.findById(req.params.id);
      if (!existing) return next(); // 404 sẽ được crudFactory xử lý
      deviceId = deviceId || existing.deviceId;
      candidate.startTime = candidate.startTime || existing.startTime;
      candidate.endTime = candidate.endTime || existing.endTime;
      // Nếu đổi giờ trên lịch đang active, cũng cần check
      if (isTimeChange && existing.status === true) shouldCheck = true;
    }

    if (!shouldCheck) return next();

    if (!deviceId || !candidate.startTime || !candidate.endTime) return next();

    const others = await Schedule.find({
      deviceId,
      status: true,
      _id: { $ne: req.params.id || null },
    });

    for (const other of others) {
      if (rangesOverlap(candidate, other)) {
        return res.status(409).json({
          message: `Khung giờ trùng với lịch đang bật (${fmtRange(other)})`,
          conflictId: other.id,
        });
      }
    }
    next();
  } catch (err) {
    next(err);
  }
}

router.get("/", c.getAll);
router.get("/:id", c.getOne);
router.post("/", checkOverlap, c.create);
router.patch("/:id", checkOverlap, c.update);
router.delete("/:id", c.remove);

module.exports = router;
