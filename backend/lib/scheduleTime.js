// ─────────────────────────────────────────────────────────────────────────────
// scheduleTime.js — So khớp giờ cho Schedule (lịch hẹn giờ).
// startTime/endTime lưu chuỗi ISO đầy đủ nhưng CHỈ phần giờ:phút được dùng để so
// khớp (dailyRepeat luôn true với lịch tạo qua UI mới). Hỗ trợ khung giờ qua đêm
// (endTime giờ:phút < startTime giờ:phút).
// ─────────────────────────────────────────────────────────────────────────────

// Quy đổi 1 chuỗi ISO datetime -> số phút trong ngày (0-1439), theo giờ local
// của máy chạy Node (server thật chạy giờ Việt Nam). Dùng local getter để
// nhất quán với `now` (new Date() thực tế) trong isActiveNow bên dưới.
function toMinutesOfDay(iso) {
  const d = new Date(iso);
  return d.getHours() * 60 + d.getMinutes();
}

// schedule có đang trong khung giờ active tại thời điểm `now` không.
// Xử lý wrap-around: nếu start > end (giờ:phút), khung giờ kéo dài qua nửa đêm.
function isActiveNow(schedule, now) {
  const s = toMinutesOfDay(schedule.startTime);
  const e = toMinutesOfDay(schedule.endTime);
  const n = now.getHours() * 60 + now.getMinutes();
  return s <= e ? n >= s && n < e : n >= s || n < e;
}

// Triển khai đơn giản: kiểm tra giao nhau bằng cách thử cả 2 lần
// dịch pha (offset 0 và +1440) cho từng khung, đủ để bắt mọi trường hợp wrap.
function rangesOverlap(a, b) {
  const segs = (sch) => {
    const s = toMinutesOfDay(sch.startTime);
    const e = toMinutesOfDay(sch.endTime);
    return s <= e ? [[s, e]] : [[s, e + 1440]];
  };
  const segsA = segs(a);
  const segsB = segs(b);
  for (const [as, ae] of segsA) {
    for (const [bs, be] of segsB) {
      for (const shift of [0, -1440, 1440]) {
        const bs2 = bs + shift, be2 = be + shift;
        if (as < be2 && bs2 < ae) return true;
      }
    }
  }
  return false;
}

function fmtRange(schedule) {
  const fmt = (iso) => {
    const d = new Date(iso);
    const hh = String(d.getHours()).padStart(2, "0");
    const mm = String(d.getMinutes()).padStart(2, "0");
    return `${hh}:${mm}`;
  };
  return `${fmt(schedule.startTime)} – ${fmt(schedule.endTime)}`;
}

module.exports = { isActiveNow, rangesOverlap, fmtRange, toMinutesOfDay };
