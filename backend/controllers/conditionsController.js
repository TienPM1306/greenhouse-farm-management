// conditionsController — quản lý điều kiện tự động hóa người dùng thêm vào (Bảng 3.4).
const Condition = require("../models/Condition");
module.exports = require("./crudFactory")(Condition);
