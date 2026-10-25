// Collection "Sensors" — dữ liệu cảm biến (báo cáo Hình 3.32).
// Trường `data` chứa today (hiện tại) + pastDay (quá khứ), cấu trúc linh hoạt
// theo từng thiết bị IoT nên dùng kiểu Mixed.
const mongoose = require("mongoose");
const { defineModel } = require("./baseModel");

module.exports = defineModel("Sensor", {
  name: String, // tên (tùy chọn)
  gatewayId: String, // id gateway quản lý
  nodeId: String, // id node quản lý cảm biến
  sensorType: Number, // 1=nhiệt độ, 2=độ ẩm, 3=ánh sáng...
  data: { type: mongoose.Schema.Types.Mixed, default: {} }, // { today:{dataMinute,dataHour}, pastDay:[...] }
});
