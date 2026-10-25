// Collection "Node" — thông tin node = nhà kính (báo cáo Hình 3.30).
const { defineModel } = require("./baseModel");

module.exports = defineModel("Node", {
  name: { type: String, default: "" }, // tên node — RỖNG khi mới tự sinh; đặt tên sau qua web
  mac: String, // MAC thật (12 hex) — định danh node, map từ nodeMac trong node_data
  registered: { type: Boolean, default: false }, // user đã "đăng ký/đặt tên" qua web? (đặt tên = registered)
  online: { type: Boolean, default: false }, // đang phát dữ liệu? (cập nhật theo lastSeen)
  lastSeen: String, // lần cuối có node_data (ISO)
  gatewayId: String, // id gateway quản lý node
  nodePosition: String, // vị trí
  nodeCreatedDay: String, // ngày tạo
  nodeAddh: String, // địa chỉ LoRa (high)
  nodeAddl: String, // địa chỉ LoRa (low)
  nodeDesc: String, // mô tả node
  nodeAddress: String, // tọa độ (lat,lng)
});
