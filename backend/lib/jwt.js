// lib/jwt.js — ký và xác thực JWT cho phiên đăng nhập.
// Secret lấy từ env JWT_SECRET (KHÔNG hardcode). Token hết hạn sau 7 ngày.
const jwt = require("jsonwebtoken");

const EXPIRES_IN = "7d";

function getSecret() {
  const s = process.env.JWT_SECRET;
  if (!s) throw new Error("Thiếu JWT_SECRET trong môi trường");
  return s;
}

// payload nên nhỏ gọn: { userId, username }
function signToken(payload) {
  return jwt.sign(payload, getSecret(), { expiresIn: EXPIRES_IN });
}

// Trả payload nếu hợp lệ; throw nếu token sai chữ ký hoặc hết hạn.
function verifyToken(token) {
  return jwt.verify(token, getSecret());
}

module.exports = { signToken, verifyToken };
