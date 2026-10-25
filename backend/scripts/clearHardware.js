// clearHardware.js — Làm sạch dữ liệu phần cứng GIẢ để DB phản ánh đúng logic đăng ký.
// XÓA hết: gateways, newGateways, nodes, sensors, devices, conditions, schedules, notifies.
// GIỮ: users (tài khoản admin để đăng nhập).
// Sau đó dữ liệu thật sẽ TỰ SINH từ MQTT (GĐ 3b auto-provision): node mới = CHƯA ĐẶT TÊN,
// đặt tên sau qua web.
// Chạy: node scripts/clearHardware.js
require("dotenv").config();
const mongoose = require("mongoose");

const MONGODB_URI =
  process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

// Các collection phần cứng cần làm sạch (KHÔNG đụng users)
const HARDWARE = {
  gateways: require("../models/Gateway"),
  newGateways: require("../models/NewGateway"),
  nodes: require("../models/Node"),
  devices: require("../models/Device"),
  sensors: require("../models/Sensor"),
  schedules: require("../models/Schedule"),
  conditions: require("../models/Condition"),
  notifies: require("../models/Notify"),
};

async function main() {
  await mongoose.connect(MONGODB_URI);
  console.log("✓ Kết nối MongoDB:", MONGODB_URI);
  for (const [name, Model] of Object.entries(HARDWARE)) {
    const r = await Model.deleteMany({});
    console.log(`  ${name.padEnd(12)} : xóa ${r.deletedCount}`);
  }
  const userCount = await require("../models/User").countDocuments();
  console.log(`  users (giữ)  : ${userCount}`);
  await mongoose.disconnect();
  console.log("✓ Đã làm sạch dữ liệu phần cứng. DB sẵn sàng cho dữ liệu thật.");
}

main().catch((err) => {
  console.error("✗ Lỗi:", err.message);
  process.exit(1);
});
