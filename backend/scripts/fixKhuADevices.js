// fixKhuADevices.js — TẠM (dev): tạo lại đúng 3 device cho node Khu A với pin chuẩn.
// Xóa hết device của node Khu A (mac ECE3347B678C) rồi tạo 3: pin 1=Đèn, 2=Máy bơm, 3=Quạt.
require("dotenv").config();
const mongoose = require("mongoose");

const URI = process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

async function main() {
  await mongoose.connect(URI);
  const db = mongoose.connection.db;

  const node = await db.collection("nodes").findOne({ mac: "ECE3347B678C" });
  if (!node) { console.log("Không thấy node ECE3347B678C"); return mongoose.disconnect(); }

  const del = await db.collection("devices").deleteMany({ nodeId: node._id });
  console.log(`Xóa ${del.deletedCount} device cũ của Khu A`);

  // _id phải là STRING (baseModel khai _id:String) — nếu để insertMany tự sinh ObjectId thì
  // findById/PATCH/DELETE qua API sẽ 404 (mongoose không cast string ↔ ObjectId).
  const rnd = () => Math.random().toString(36).slice(2, 12);
  const defs = [
    { name: "Đèn", pin: "1" },
    { name: "Máy bơm", pin: "2" },
    { name: "Quạt", pin: "3" },
  ].map((d) => ({ _id: `dev-${node.mac}-${d.pin}-${rnd()}`, gatewayId: node.gatewayId, nodeId: node._id, name: d.name, pin: d.pin, status: false }));
  await db.collection("devices").insertMany(defs);
  console.log(`Tạo lại ${defs.length} device (pin 1/2/3, _id String)`);

  const left = await db.collection("devices").find({ nodeId: node._id }).toArray();
  console.log("Khu A giờ có:", left.map((d) => `${d.name}(pin=${d.pin})`).join(", "));
  await mongoose.disconnect();
}
main().catch((e) => { console.error("✗", e.message); process.exit(1); });
