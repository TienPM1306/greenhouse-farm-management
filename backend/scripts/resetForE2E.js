// resetForE2E.js — TẠM (dev): dọn để test lại luồng tạo dự án từ web.
// Xóa: gateway dự án David + node thật ECE3347B678C + sensors của nó.
// GIỮ: newgateways (Lucy) để web "Cấu hình mới" lại; users; node mẫu Xà lách.
// Chạy: node scripts/resetForE2E.js
require("dotenv").config();
const mongoose = require("mongoose");

const URI = process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

async function main() {
  await mongoose.connect(URI);
  const db = mongoose.connection.db;

  // Xóa gateway dự án có mac D48AFCA6BC5C (David)
  const g = await db.collection("gateways").deleteMany({ mac: "D48AFCA6BC5C" });
  console.log(`gateways: xóa ${g.deletedCount} (David)`);

  // Xóa node thật + sensors/devices của nó (mac ECE3347B678C)
  const node = await db.collection("nodes").findOne({ mac: "ECE3347B678C" });
  if (node) {
    await db.collection("sensors").deleteMany({ nodeId: node._id });
    await db.collection("devices").deleteMany({ nodeId: node._id });
    await db.collection("nodes").deleteOne({ _id: node._id });
    console.log(`node ECE3347B678C + sensors/devices: đã xóa`);
  } else {
    console.log("node ECE3347B678C: không có (đã sạch)");
  }

  // Đưa newgateway Lucy về CHƯA cấu hình (bỏ tên + configured) để cấu hình lại từ đầu.
  const ng = await db.collection("newgateways").updateMany(
    { mac: "D48AFCA6BC5C" },
    { $set: { configured: false }, $unset: { gatewayName: "" } }
  );
  console.log(`newgateways: reset ${ng.modifiedCount} về chưa cấu hình (Lucy)`);

  // Báo cáo còn lại
  for (const c of ["gateways", "newgateways", "nodes", "sensors"]) {
    console.log(`  ${c}: còn ${await db.collection(c).countDocuments()}`);
  }
  await mongoose.disconnect();
  console.log("✓ Xong. Giờ vào web: Cấu hình mới (Lucy) → tạo dự án → quan sát register tự động.");
}
main().catch((e) => { console.error("✗", e.message); process.exit(1); });
