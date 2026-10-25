// devicesController — thêm/lấy/sửa/xóa thiết bị (Bảng 3.4).
const Device = require("../models/Device");
module.exports = require("./crudFactory")(Device);
