// Collection "User" — thông tin người dùng (báo cáo mục 3.3.2.3, Hình 3.28).
const { defineModel } = require("./baseModel");

module.exports = defineModel("User", {
  name: String, // tên người dùng
  birth: String, // ngày sinh
  phone: String, // số điện thoại
  email: String,
  username: { type: String, required: true }, // tài khoản
  password: String, // mật khẩu (user đăng nhập Google/GitHub không có password)
  provider: { type: String, default: "local" }, // "local" | "google" | "github"
  providerId: String, // id của user bên provider (đối chiếu)
  avatar: String, // hình đại diện (base64/URL)
  home: String, // địa chỉ
  gatewayId: [String], // các gateway mà user quản lý
});
