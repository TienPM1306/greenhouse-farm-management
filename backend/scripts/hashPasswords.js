// scripts/hashPasswords.js — migrate 1 lần: hash password plaintext của user hiện có.
// Idempotent: bỏ qua user có password đã là hash bcrypt ($2a/$2b...).
// Chạy: MONGODB_URI="<atlas>" node scripts/hashPasswords.js
require("dotenv").config();
const mongoose = require("mongoose");
const bcrypt = require("bcryptjs");
const User = require("../models/User");

const MONGODB_URI = process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";
const SALT_ROUNDS = 10;

async function main() {
  await mongoose.connect(MONGODB_URI);
  console.log("✓ Đã kết nối MongoDB");
  const users = await User.find({});
  let hashed = 0, skipped = 0;
  for (const u of users) {
    if (typeof u.password === "string" && u.password.startsWith("$2")) {
      skipped++;
      continue;
    }
    u.password = await bcrypt.hash(String(u.password), SALT_ROUNDS);
    await u.save();
    hashed++;
    console.log(`  hashed: ${u.username}`);
  }
  console.log(`✓ Xong. Hash mới: ${hashed}, bỏ qua (đã hash): ${skipped}`);
  await mongoose.disconnect();
}

main().catch((e) => { console.error("✗ Lỗi:", e.message); process.exit(1); });
