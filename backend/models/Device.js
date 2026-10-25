// Collection "Devices" — thiết bị điện điều khiển được (báo cáo Hình 3.31).
const { defineModel } = require("./baseModel");

module.exports = defineModel("Device", {
  gatewayId: String, // id gateway quản lý
  nodeId: String, // id node quản lý thiết bị
  name: String, // tên thiết bị
  pin: String, // loại/chân thiết bị (relay pin)
  description: String, // mô tả
  status: { type: Boolean, default: false }, // trạng thái bật/tắt
  mode: { type: String, default: "manual" }, // "manual" | "auto" — cho đèn (pin "1") + quạt (pin "3")
});
