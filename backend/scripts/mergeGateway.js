// mergeGateway.js — TẠM (dev): gộp gateway lệch do web tạo (thiếu mac) và gateway
// mqttHandler tự tạo (theo mac). Giữ record dự án của web, gán mac + registered, trỏ
// node/sensor/device sang dự án đó, xóa record mqtt thừa.
// Chạy: node scripts/mergeGateway.js <KEEP_ID> <DROP_ID> <MAC>
require("dotenv").config();
const mongoose = require("mongoose");

const MONGODB_URI =
  process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

const KEEP = process.argv[2]; // record dự án của web (giữ)
const DROP = process.argv[3]; // record mqttHandler tạo (xóa)
const MAC = (process.argv[4] || "").toUpperCase();

async function main() {
  if (!KEEP || !DROP || !MAC) {
    console.error("Cách dùng: node scripts/mergeGateway.js <KEEP_ID> <DROP_ID> <MAC>");
    process.exit(1);
  }
  await mongoose.connect(MONGODB_URI);
  const db = mongoose.connection.db;

  // 1) Gán mac + registered cho record giữ lại (dự án của web)
  await db.collection("gateways").updateOne(
    { _id: KEEP },
    { $set: { mac: MAC, registered: true, lastSeen: new Date().toISOString() } }
  );
  console.log(`✓ Gán mac=${MAC}, registered=true cho gateway giữ lại ${KEEP}`);

  // 2) Trỏ mọi node/sensor/device từ DROP → KEEP
  for (const coll of ["nodes", "sensors", "devices"]) {
    const r = await db.collection(coll).updateMany(
      { gatewayId: DROP },
      { $set: { gatewayId: KEEP } }
    );
    console.log(`  ${coll}: trỏ ${r.modifiedCount} doc sang ${KEEP}`);
  }

  // 3) Xóa record gateway thừa
  const d = await db.collection("gateways").deleteOne({ _id: DROP });
  console.log(`✓ Xóa gateway thừa ${DROP}: deleted=${d.deletedCount}`);

  await mongoose.disconnect();
  console.log("✓ Gộp xong.");
}

main().catch((e) => {
  console.error("✗ Lỗi:", e.message);
  process.exit(1);
});
