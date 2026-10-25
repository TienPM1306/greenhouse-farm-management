const router = require("express").Router();
const c = require("../controllers/gatewaysController");

router.get("/", c.getAll);
router.get("/:id", c.getOne);
router.post("/", c.create);
router.patch("/:id", c.update);
router.delete("/:id", c.remove);

// GĐ3c — gửi lệnh config xuống gateway (đăng ký/ping/hủy đăng ký)
router.post("/:id/config", c.configGateway);
router.post("/:id/scan-nodes", c.scanNodes);

module.exports = router;
