// newGatewayController — quản lý Gateway chưa xử lý: thêm mới, sửa thông tin rồi xóa đi
// sau khi đã được cấu hình thành gateway chính thức (Bảng 3.4).
const NewGateway = require("../models/NewGateway");
module.exports = require("./crudFactory")(NewGateway);
