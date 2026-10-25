// Collection "newGateways" — gateway phần cứng vừa lên mạng nhưng CHƯA được cấu hình
// (báo cáo: newGatewayController quản lý gateway chưa xử lý). Dữ liệu đến từ firmware
// có thể khác nhau nên cho phép trường linh hoạt (strict: false).
const { defineModel } = require("./baseModel");

module.exports = defineModel(
  "NewGateway",
  {
    gatewayName: String, // tên/định danh gateway báo lên
    mac: String, // địa chỉ MAC phần cứng (12 hex)
    ssid: String, // WiFi SSID gateway báo kèm
    info: Object, // payload thô kèm theo
    createdAt: String, // thời điểm ghi nhận
  },
  { strict: false }
);
