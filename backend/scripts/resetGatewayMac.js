// resetGatewayMac.js — TẠM (dev): dọn sạch mọi gateway + node/sensor/device theo 1 MAC,
// đưa về trạng thái "chưa cấu hình". Node thật sẽ được mqttHandler auto-tạo lại SẠCH khi
// nó tiếp tục phát node_data. Dùng khi data bị kẹt/trùng sau "Cấu hình lại".
// Chạy: node scripts/resetGatewayMac.js <MAC>   (mặc định D48AFCA6BC5C)
require("dotenv").config();
const mongoose = require("mongoose");
const URI = process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";
const MAC = (process.argv[2] || "D48AFCA6BC5C").toUpperCase();

async function main() {
  await mongoose.connect(URI);
  const db = mongoose.connection.db;

  // Mọi gateway có mac này
  const gws = await db.collection("gateways").find({ mac: MAC }).toArray();
  const gwIds = gws.map((g) => g._id);
  console.log(`Gateway mac=${MAC}: ${gwIds.length} record`);

  // Node thuộc các gateway đó
  const nodes = await db.collection("nodes").find({ gatewayId: { $in: gwIds } }).toArray();
  const nodeIds = nodes.map((n) => n._id);

  // Xóa device + sensor của node, rồi node, rồi gateway
  if (nodeIds.length) {
    const d = await db.collection("devices").deleteMany({ nodeId: { $in: nodeIds } });
    const s = await db.collection("sensors").deleteMany({ nodeId: { $in: nodeIds } });
    await db.collection("nodes").deleteMany({ _id: { $in: nodeIds } });
    console.log(`Node: xóa ${nodeIds.length} | device ${d.deletedCount} | sensor ${s.deletedCount}`);
  } else {
    console.log("Không có node thuộc gateway này.");
  }
  await db.collection("devices").deleteMany({ gatewayId: { $in: gwIds } });
  await db.collection("sensors").deleteMany({ gatewayId: { $in: gwIds } });
  const g = await db.collection("gateways").deleteMany({ mac: MAC });
  console.log(`Gateway: xóa ${g.deletedCount}`);

  // Dọn newGateways của mac này về sạch (để cấu hình lại từ đầu)
  const ng = await db.collection("newgateways").deleteMany({ mac: MAC });
  console.log(`newGateways mac=${MAC}: xóa ${ng.deletedCount}`);

  console.log("\n--- Còn lại ---");
  for (const c of ["gateways", "newgateways", "nodes", "sensors", "devices"]) {
    console.log(`  ${c}: ${await db.collection(c).countDocuments()}`);
  }
  await mongoose.disconnect();
  console.log("✓ Xong. Gateway thật phát lại → sẽ publish newgateway (chưa đăng ký) → Cấu hình mới.");
}
main().catch((e) => { console.error("✗", e.message); process.exit(1); });
