const router = require("express").Router();
const c = require("../controllers/nodeController");
const { controlNode } = require("../controllers/controlController");

router.get("/", c.getAll);
router.get("/:id", c.getOne);
router.post("/", c.create);
router.patch("/:id", c.update);
router.delete("/:id", c.remove);

// GĐ3c — CỔNG điều khiển: chỉ node đã đăng ký mới điều khiển được thiết bị NodeControl
router.post("/:id/control", controlNode);

module.exports = router;
