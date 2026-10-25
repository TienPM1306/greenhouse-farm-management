// Helper tạo model mongoose theo quy ước chung của dự án.
// - _id dạng String để GIỮ id cũ ("gw01", "nk1", "user1"...) từ db.json và các quan hệ
//   gatewayId/nodeId tham chiếu chéo vẫn đúng.
// - Tạo mới (POST) không truyền id -> tự sinh id chuỗi.
// - toJSON trả về `id` thay cho `_id` để khớp frontend cũ (json-server cũng dùng `id`).
const mongoose = require("mongoose");

function defineModel(name, fields, options = {}) {
  const schema = new mongoose.Schema(
    {
      _id: { type: String, default: () => new mongoose.Types.ObjectId().toString() },
      ...fields,
    },
    {
      versionKey: false,
      minimize: false, // giữ nguyên object rỗng (vd data sensor)
      ...options,
      toJSON: {
        transform(_doc, ret) {
          ret.id = ret._id;
          delete ret._id;
          return ret;
        },
      },
    }
  );
  return mongoose.model(name, schema);
}

module.exports = { defineModel };
