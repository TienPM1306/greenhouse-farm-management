// cleanSeedKeepReal.js — TẠM (dev): xóa sạch data MẪU SEED còn sót, giữ data THẬT (phần cứng).
// GIỮ: node Khu A (ECE3347B678C) + 3 sensor + 3 device của nó; gateway David; user admin.
// XÓA: node mẫu "Xà lách" (gatewayId=null) + device của nó; toàn bộ schedules/conditions/notifies mẫu.
require("dotenv").config();
const mongoose = require("mongoose");
const URI = process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

async function main() {
  await mongoose.connect(URI);
  const db = mongoose.connection.db;

  // 1) Node mẫu = node có gatewayId null (không thuộc gateway thật nào)
  const orphanNodes = await db.collection("nodes").find({ gatewayId: null }).toArray();
  const orphanIds = orphanNodes.map((n) => n._id);
  if (orphanIds.length) {
    const ds = await db.collection("devices").deleteMany({ nodeId: { $in: orphanIds } });
    const ss = await db.collection("sensors").deleteMany({ nodeId: { $in: orphanIds } });
    const ns = await db.collection("nodes").deleteMany({ _id: { $in: orphanIds } });
    console.log(`Node mẫu (gatewayId=null): xóa ${ns.deletedCount} node, ${ds.deletedCount} device, ${ss.deletedCount} sensor`);
  } else {
    console.log("Không có node mẫu.");
  }

  // 2) schedules / conditions / notifies: toàn bộ là mẫu seed → xóa sạch
  for (const c of ["schedules", "conditions", "notifies"]) {
    const r = await db.collection(c).deleteMany({});
    console.log(`${c}: xóa ${r.deletedCount}`);
  }

  // Báo cáo còn lại
  console.log("--- Còn lại (data thật) ---");
  for (const c of ["users", "gateways", "newgateways", "nodes", "sensors", "devices", "schedules", "conditions", "notifies"]) {
    console.log(`  ${c}: ${await db.collection(c).countDocuments()}`);
  }
  await mongoose.disconnect();
  console.log("✓ Xong.");
}
main().catch((e) => { console.error("✗", e.message); process.exit(1); });
