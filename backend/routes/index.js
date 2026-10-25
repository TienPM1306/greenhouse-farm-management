// Gộp toàn bộ router, server.js sẽ mount dưới prefix /v1 (khớp backend gốc .../v1).
const router = require("express").Router();

router.use("/users", require("./users"));
router.use("/gateways", require("./gateways"));
router.use("/nodes", require("./nodes"));
router.use("/devices", require("./devices"));
router.use("/sensors", require("./sensors"));
router.use("/schedules", require("./schedules"));
router.use("/conditions", require("./conditions"));
router.use("/newGateways", require("./newGateways"));
router.use("/notifies", require("./notifies"));

module.exports = router;
