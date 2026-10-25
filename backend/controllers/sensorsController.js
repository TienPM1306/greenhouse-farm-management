// sensorsController — thêm/lấy dữ liệu hiện tại & quá khứ/sửa/xóa cảm biến (Bảng 3.4).
const Sensor = require("../models/Sensor");
module.exports = require("./crudFactory")(Sensor);
