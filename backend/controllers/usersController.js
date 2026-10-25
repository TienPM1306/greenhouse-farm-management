// usersController — quản lý người dùng: đăng ký, đăng nhập, đổi mật khẩu, hồ sơ (Bảng 3.4).
const User = require("../models/User");
const crud = require("./crudFactory")(User);
const bcrypt = require("bcryptjs");
const { signToken } = require("../lib/jwt");
const Otp = require("../models/Otp");
const { sendOtpMail } = require("../lib/mailer");
const SALT_ROUNDS = 10;
const OTP_TTL_MS = 5 * 60 * 1000; // 5 phút

// Sinh mã OTP 6 chữ số dạng chuỗi (giữ số 0 ở đầu).
function genOtpCode() {
  return String(Math.floor(100000 + Math.random() * 900000));
}

// Bỏ password khỏi object trả về
function safe(userDoc) {
  const obj = userDoc.toJSON();
  delete obj.password;
  return obj;
}

// POST /users/register — đăng ký tài khoản mới
async function register(req, res, next) {
  try {
    const { username } = req.body;
    if (!username || !req.body.password) {
      return res.status(400).json({ message: "Thiếu username hoặc password" });
    }
    const existed = await User.findOne({ username });
    if (existed) return res.status(409).json({ message: "Username đã tồn tại" });

    const body = { ...req.body };
    if (body.id && !body._id) {
      body._id = body.id;
      delete body.id;
    }
    body.password = await bcrypt.hash(String(body.password), SALT_ROUNDS);
    const user = await User.create(body);
    res.status(201).json(safe(user));
  } catch (err) {
    next(err);
  }
}

// POST /users/login — đăng nhập bằng username + password, trả JWT + user
async function login(req, res, next) {
  try {
    const { username, password } = req.body;
    if (!username || !password) {
      return res.status(400).json({ message: "Thiếu username hoặc password" });
    }
    const user = await User.findOne({ username });
    if (!user) {
      return res.status(401).json({ message: "Sai tài khoản hoặc mật khẩu" });
    }
    const ok = await bcrypt.compare(String(password), user.password);
    if (!ok) {
      return res.status(401).json({ message: "Sai tài khoản hoặc mật khẩu" });
    }
    const token = signToken({ userId: user._id, username: user.username });
    res.json({ token, user: safe(user) });
  } catch (err) {
    next(err);
  }
}

// POST /users/forgot-password — sinh OTP 6 số, lưu Mongo (hết hạn 5 phút), gửi qua email thật.
// Báo lỗi rõ nếu username + email không khớp (cố ý cho DATN dễ demo).
async function forgotPassword(req, res, next) {
  try {
    const { username, email } = req.body || {};
    if (!username || !email) {
      return res.status(400).json({ message: "Thiếu tên đăng nhập hoặc email" });
    }
    const user = await User.findOne({ username });
    if (!user || !user.email || user.email.toLowerCase() !== String(email).toLowerCase()) {
      return res.status(404).json({ message: "Tên đăng nhập và email không khớp" });
    }
    // Xóa OTP cũ chưa dùng của user này (tránh nhiều mã còn sống cùng lúc).
    await Otp.deleteMany({ username, used: false });
    const code = genOtpCode();
    await Otp.create({ username, email: user.email, code, used: false, createdAt: new Date() });
    // Gửi mail — nếu lỗi SMTP thì báo 502 để người dùng biết chưa nhận được mã.
    try {
      await sendOtpMail(user.email, code);
    } catch (mailErr) {
      console.error("Gửi OTP thất bại:", mailErr.message);
      return res.status(502).json({ message: "Không gửi được email. Thử lại sau." });
    }
    res.json({ message: "Đã gửi mã OTP tới email của bạn", sent: true, username: user.username });
  } catch (err) {
    next(err);
  }
}

// POST /users/verify-otp — kiểm OTP: khớp code, chưa dùng, chưa hết hạn 5 phút.
// KHÔNG đổi mật khẩu ở bước này, chỉ xác nhận mã đúng để frontend mở form đặt MK mới.
async function verifyOtp(req, res, next) {
  try {
    const { username, code } = req.body || {};
    if (!username || !code) {
      return res.status(400).json({ message: "Thiếu tên đăng nhập hoặc mã OTP" });
    }
    const otp = await Otp.findOne({ username, code: String(code), used: false });
    if (!otp) {
      return res.status(400).json({ message: "Mã OTP không đúng" });
    }
    if (Date.now() - new Date(otp.createdAt).getTime() > OTP_TTL_MS) {
      return res.status(400).json({ message: "Mã OTP đã hết hạn" });
    }
    res.json({ message: "Xác thực OTP thành công", verified: true });
  } catch (err) {
    next(err);
  }
}

// POST /users/reset-password — đặt MK mới sau khi có OTP hợp lệ. Hash bcrypt + xóa OTP (dùng 1 lần).
async function resetPassword(req, res, next) {
  try {
    const { username, code, newPassword } = req.body || {};
    if (!username || !code || !newPassword) {
      return res.status(400).json({ message: "Thiếu thông tin đặt lại mật khẩu" });
    }
    if (String(newPassword).length < 4) {
      return res.status(400).json({ message: "Mật khẩu mới phải có ít nhất 4 ký tự" });
    }
    const otp = await Otp.findOne({ username, code: String(code), used: false });
    if (!otp) {
      return res.status(400).json({ message: "Mã OTP không đúng" });
    }
    if (Date.now() - new Date(otp.createdAt).getTime() > OTP_TTL_MS) {
      return res.status(400).json({ message: "Mã OTP đã hết hạn" });
    }
    const user = await User.findOne({ username });
    if (!user) {
      return res.status(404).json({ message: "Không tìm thấy người dùng" });
    }
    user.password = await bcrypt.hash(String(newPassword), SALT_ROUNDS);
    await user.save();
    otp.used = true;
    await otp.save();
    await Otp.deleteMany({ username }); // dọn mọi OTP còn lại của user
    res.json({ message: "Đặt lại mật khẩu thành công", username: user.username });
  } catch (err) {
    next(err);
  }
}

module.exports = { ...crud, register, login, forgotPassword, verifyOtp, resetPassword };
