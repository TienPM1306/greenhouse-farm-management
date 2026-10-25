// Collection "conditions" — điều kiện tự động hóa bật/tắt thiết bị (báo cáo mục 3.4.2.5).
const { defineModel } = require("./baseModel");

module.exports = defineModel("Condition", {
  gatewayId: String,
  nodeId: String,
  deviceName: String, // thiết bị bị điều khiển
  devicePin: String,
  status: { type: Boolean, default: true }, // điều kiện đang bật?
  sensorType: Number, // loại cảm biến tham chiếu (1/2/3)
  minValue: Number, // ngưỡng dưới
  maxValue: Number, // ngưỡng trên
});
