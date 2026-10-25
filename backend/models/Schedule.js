// Collection "schedules" — lịch hẹn giờ bật/tắt thiết bị (báo cáo mục 3.4.2.5).
const { defineModel } = require("./baseModel");

module.exports = defineModel("Schedule", {
  gatewayId: String,
  nodeId: String,
  deviceId: String, // tham chiếu Device._id — cho phép query /schedules?deviceId=X
  deviceName: String, // tên thiết bị áp dụng
  devicePin: String, // chân thiết bị
  status: { type: Boolean, default: false }, // lịch đang Auto? (mặc định Dừng khi tạo mới)
  startTime: String, // thời điểm bắt đầu (ISO) — chỉ phần giờ:phút được dùng khi dailyRepeat=true
  endTime: String, // thời điểm kết thúc (ISO) — endTime < startTime (giờ:phút) = khung giờ qua đêm
  dailyRepeat: { type: Boolean, default: true }, // lặp hằng ngày
});
