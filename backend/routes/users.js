const router = require("express").Router();
const c = require("../controllers/usersController");

// Đăng nhập / đăng ký / quên mật khẩu (đặt trước /:id để không bị nuốt route)
router.post("/login", c.login);
router.post("/register", c.register);
router.post("/forgot-password", c.forgotPassword);
router.post("/verify-otp", c.verifyOtp);
router.post("/reset-password", c.resetPassword);

router.get("/", c.getAll);
router.get("/:id", c.getOne);
router.post("/", c.create);
router.patch("/:id", c.update);
router.delete("/:id", c.remove);

module.exports = router;
