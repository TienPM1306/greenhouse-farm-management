// cleanKeepOne.js — TẠM (dev): làm sạch greenhouse_datn, giữ lại 1 mẫu/collection.
// - users, gateways: giữ nguyên (đã 1 doc, để login + tham chiếu).
// - nodes/sensors/devices/schedules/conditions/notifies: giữ document _id chỉ định.
// - newgateways: XÓA SẠCH (gateway thật sắp publish sẽ tạo mới).
// Chạy: node scripts/cleanKeepOne.js
require("dotenv").config();
const mongoose = require("mongoose");

const MONGODB_URI =
  process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

// collection (tên thật trong Mongo) -> _id mẫu giữ lại (null = xóa sạch)
const KEEP = {
  nodes: "O0ExU7Q59bQ",
  sensors: "nk1-s1",
  devices: "5Ty3a9MQIBk",
  schedules: "nk1-sch-light",
  conditions: "nk1-c-fan",
  notifies: "ntf1",
  newgateways: null, // xóa sạch
};

async function main() {
  await mongoose.connect(MONGODB_URI);
  console.log("✓ Đã kết nối:", MONGODB_URI);
  const db = mongoose.connection.db;

  for (const [coll, keepId] of Object.entries(KEEP)) {
    const filter = keepId ? { _id: { $ne: keepId } } : {};
    const r = await db.collection(coll).deleteMany(filter);
    const left = await db.collection(coll).countDocuments();
    console.log(
      `  ${coll}: xóa ${r.deletedCount}${keepId ? ` (giữ ${keepId})` : " (sạch)"} → còn ${left}`
    );
  }

  // báo cáo các collection KHÔNG đụng
  for (const coll of ["users", "gateways"]) {
    const left = await db.collection(coll).countDocuments();
    console.log(`  ${coll}: giữ nguyên → ${left}`);
  }

  await mongoose.disconnect();
  console.log("✓ Xong.");
}

main().catch((e) => {
  console.error("✗ Lỗi:", e.message);
  process.exit(1);
});
