// ─────────────────────────────────────────────────────────────────────────────
// server.js — Chương trình chính của Backend Application (báo cáo mục 3.3.2.3).
// Nhiệm vụ: khởi tạo kết nối MongoDB, kiểm tra API Key, mount các routes dưới /v1,
// và khởi động cầu nối MQTT (GĐ 3: subscribe dữ liệu + publish lệnh điều khiển/config).
// Luồng khởi động theo Hình 3.25: kết nối MongoDB sai -> dừng; đúng -> phục vụ API.
// ─────────────────────────────────────────────────────────────────────────────
require("dotenv").config();
const path = require("path");
const express = require("express");
const cors = require("cors");
const cookieParser = require("cookie-parser");
const mongoose = require("mongoose");
const apiRoutes = require("./routes");
const authRoutes = require("./routes/auth");
const { startMqtt, isMqttConnected } = require("./mqttHandler");
const { startScheduler } = require("./scheduler");
const { sseHandler } = require("./sseHub");
const { requireAuth } = require("./lib/auth");

const PORT = process.env.PORT || 4000;
const MONGODB_URI = process.env.MONGODB_URI || "mongodb://localhost:27017/greenhouse_datn";

const app = express();
app.use(cors());
app.use(cookieParser()); // đọc cookie oauth_state cho luồng social login
app.use(express.json({ limit: "5mb" })); // avatar base64 có thể lớn

// Health-check công khai (không cần API Key) để kiểm tra server sống + trạng thái MQTT.
// Chuyển sang /api-status vì "/" nay phục vụ trang frontend (index.html).
app.get("/api-status", (_req, res) => {
  res.json({
    status: "ok",
    service: "greenhouse-backend",
    api: "/v1",
    mqtt: isMqttConnected() ? "connected" : "disconnected",
  });
});

// SSE realtime (ACK/timeout relay). Route riêng, KHÔNG qua middleware x-api-key header bên dưới
// vì trình duyệt EventSource không gửi được custom header — tự xác thực bằng ?key= trong sseHub.js.
app.get("/v1/events", sseHandler);

// ── Endpoint auth PUBLIC (chưa đăng nhập vẫn gọi được): login/register/quên-đặt lại MK ──
// Mount TRƯỚC middleware requireAuth để không bị chặn.
const usersController = require("./controllers/usersController");
app.post("/v1/users/login", usersController.login);
app.post("/v1/users/register", usersController.register);
app.post("/v1/users/forgot-password", usersController.forgotPassword);
app.post("/v1/users/verify-otp", usersController.verifyOtp);
app.post("/v1/users/reset-password", usersController.resetPassword);

// ── OAuth social login (Google/GitHub) PUBLIC — mount trước requireAuth ──
app.use("/v1/auth", authRoutes);

// ── Bảo mật JWT: mọi request /v1 còn lại phải kèm Authorization: Bearer <token> ──
app.use("/v1", requireAuth);

// ── Mount toàn bộ REST API dưới /v1 ──
app.use("/v1", apiRoutes);

// ── Phục vụ frontend tĩnh (same-origin): backend host luôn dashboard ──
// Đặt SAU /v1 để API được ưu tiên. Thư mục frontend/ nằm cạnh server.js trong repo deploy.
const FRONTEND_DIR = path.join(__dirname, "frontend");
app.use(express.static(FRONTEND_DIR));
// Vào "/" → mở index.html (index.html tự redirect sang trang login/dashboard)
app.get("/", (_req, res) => {
  res.sendFile(path.join(FRONTEND_DIR, "index.html"));
});

// 404 cho route API chưa khai báo (chỉ áp cho /v1; các path khác đã qua static ở trên)
app.use((_req, res) => {
  res.status(404).json({ message: "Không tìm thấy endpoint" });
});

// Xử lý lỗi tập trung -> 500 ERROR (Hình 3.26)
app.use((err, _req, res, _next) => {
  console.error("Lỗi:", err.message);
  res.status(500).json({ message: "Lỗi máy chủ", error: err.message });
});

// ── Khởi động: kết nối MongoDB trước, thành công mới lắng nghe HTTP ──
mongoose
  .connect(MONGODB_URI)
  .then(() => {
    console.log("✓ Đã kết nối MongoDB:", MONGODB_URI);
    const server = app.listen(PORT, () => {
      console.log(`✓ Backend chạy tại http://localhost:${PORT}  (API: /v1)`);
    });
    server.on("error", (err) => {
      if (err.code === "EADDRINUSE") {
        console.error(`✗ Cổng ${PORT} đang bận — có một server khác đang chạy. Dừng nó (hoặc đổi PORT trong .env) rồi chạy lại.`);
      } else {
        console.error("✗ Lỗi server:", err.message);
      }
      process.exit(1);
    });
    // GĐ 3b: nhận MQTT từ Gateway → ghi DB (chỉ đọc/ghi, không điều khiển phần cứng)
    startMqtt();
    // Lịch hẹn giờ: quét mỗi phút, tự gửi lệnh bật/tắt đúng giờ
    startScheduler();
  })
  .catch((err) => {
    console.error("✗ Không kết nối được MongoDB:", err.message);
    process.exit(1);
  });
