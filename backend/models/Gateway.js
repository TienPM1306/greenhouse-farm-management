// Collection "Gateways" — thông tin gateway (báo cáo Hình 3.29).
const { defineModel } = require("./baseModel");

module.exports = defineModel("Gateway", {
  userId: String, // id người dùng sở hữu
  projectName: String, // tên dự án
  gatewayName: String, // tên gateway
  gatewayAddress: String, // địa chỉ gateway
  gatewayDesc: String, // mô tả (mở rộng proj_test2)
  gatewayCreatedDay: String, // ngày tạo
  nodeId: [String], // các node thuộc gateway
  mac: String, // MAC thật (12 hex) — định danh phần cứng, map từ topic MQTT
  registered: { type: Boolean, default: false }, // đã đăng ký? (true khi đang publish node_data)
  lastSeen: String, // lần cuối có dữ liệu (ISO)
});
