// dedupeDevices.js — TẠM (dev): gỡ device TRÙNG theo (nodeId, deviceId).
// Giữ lại 1 device đầu tiên cho mỗi (nodeId, deviceId), xóa phần dư.
// Chạy: node scripts/dedupeDevices.js
require("dotenv").config();
const mongoose = require("mongoose");

const URI = process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

async function main() {
  await mongoose.connect(URI);
  const col = mongoose.connection.db.collection("devices");
  const all = await col.find({}).toArray();

  const seen = new Set();
  const toDelete = [];
  // Sắp theo _id để "giữ cái đầu" ổn định
  all.sort((a, b) => String(a._id).localeCompare(String(b._id)));
  for (const d of all) {
    const key = `${d.nodeId}|${d.deviceId}`;
    if (seen.has(key)) toDelete.push(d._id);
    else seen.add(key);
  }

  if (!toDelete.length) {
    console.log("Không có device trùng. Sạch.");
  } else {
    const r = await col.deleteMany({ _id: { $in: toDelete } });
    console.log(`Đã xóa ${r.deletedCount} device trùng. Còn lại ${await col.countDocuments()}.`);
  }
  await mongoose.disconnect();
}
main().catch((e) => { console.error("✗", e.message); process.exit(1); });
