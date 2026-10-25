// lib/auth.js — middleware xác thực JWT cho REST /v1.
// Đọc header "Authorization: Bearer <token>", verify, gắn req.userId/req.username.
const { verifyToken } = require("./jwt");

function requireAuth(req, res, next) {
  const header = req.header("Authorization") || "";
  const token = header.startsWith("Bearer ") ? header.slice(7) : null;
  if (!token) {
    return res.status(401).json({ message: "Thiếu token đăng nhập" });
  }
  try {
    const payload = verifyToken(token);
    req.userId = payload.userId;
    req.username = payload.username;
    next();
  } catch (err) {
    return res.status(401).json({ message: "Token không hợp lệ hoặc đã hết hạn" });
  }
}

module.exports = { requireAuth };
