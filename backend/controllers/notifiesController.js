// notifiesController — CRUD thông báo/cảnh báo (MỞ RỘNG proj_test2, phục vụ trang alerts).
const Notify = require("../models/Notify");
module.exports = require("./crudFactory")(Notify);
