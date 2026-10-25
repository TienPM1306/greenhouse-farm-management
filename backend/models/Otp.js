// Collection "Otp" — mã OTP 6 số cho luồng quên mật khẩu.
// TTL index tự xóa document sau 5 phút; khi verify vẫn kiểm hạn thủ công cho chắc.
const { defineModel } = require("./baseModel");

const Otp = defineModel("Otp", {
  username: { type: String, required: true },
  email: { type: String, required: true },
  code: { type: String, required: true }, // 6 chữ số dạng chuỗi (giữ số 0 đầu)
  used: { type: Boolean, default: false },
  createdAt: { type: Date, default: Date.now },
});

// TTL: Mongo tự xóa document 300 giây (5 phút) sau createdAt.
Otp.schema.index({ createdAt: 1 }, { expireAfterSeconds: 300 });

module.exports = Otp;
