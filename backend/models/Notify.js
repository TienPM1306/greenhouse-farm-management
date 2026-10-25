// Collection "notifies" — thông báo/cảnh báo hệ thống.
// (MỞ RỘNG proj_test2: báo cáo gốc không có collection này, nhưng frontend trang
//  alerts đang dùng nên giữ lại.)
const { defineModel } = require("./baseModel");

module.exports = defineModel("Notify", {
  gatewayId: String,
  nodeId: String,
  type: String, // info | warning | error
  title: String,
  message: String,
  createdAt: String,
});
