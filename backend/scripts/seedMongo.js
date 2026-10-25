// seedMongo.js — Nạp dữ liệu mẫu từ backend/db.json vào MongoDB (database greenhouse_datn).
// GIỮ nguyên id cũ ("gw01", "nk1"...) bằng cách map id -> _id, nên quan hệ gatewayId/nodeId
// vẫn đúng. Chạy: npm run seed:mongo
// CẢNH BÁO: script này XÓA sạch các collection trước khi nạp lại (chỉ dùng cho dev).
require("dotenv").config();
const fs = require("fs");
const path = require("path");
const mongoose = require("mongoose");

const MONGODB_URI =
  process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

// Ánh xạ collection (key trong db.json) -> Model
const MODELS = {
  users: require("../models/User"),
  gateways: require("../models/Gateway"),
  nodes: require("../models/Node"),
  devices: require("../models/Device"),
  sensors: require("../models/Sensor"),
  schedules: require("../models/Schedule"),
  conditions: require("../models/Condition"),
  notifies: require("../models/Notify"),
  // newGateways: bản db.json hiện chưa có -> bỏ qua nếu thiếu
  newGateways: require("../models/NewGateway"),
};

// Chuyển id -> _id để giữ id cũ
function toDocs(arr) {
  return (arr || []).map((item) => {
    const { id, ...rest } = item;
    return id ? { _id: id, ...rest } : rest;
  });
}

async function main() {
  const dbPath = path.join(__dirname, "..", "db.json");
  const raw = JSON.parse(fs.readFileSync(dbPath, "utf8"));

  await mongoose.connect(MONGODB_URI);
  console.log("✓ Đã kết nối MongoDB:", MONGODB_URI);

  for (const [key, Model] of Object.entries(MODELS)) {
    const docs = toDocs(raw[key]);
    await Model.deleteMany({});
    if (docs.length) await Model.insertMany(docs);
    console.log(`  ${key}: nạp ${docs.length} bản ghi`);
  }

  await mongoose.disconnect();
  console.log("✓ Seed xong.");
}

main().catch((err) => {
  console.error("✗ Seed lỗi:", err.message);
  process.exit(1);
});
