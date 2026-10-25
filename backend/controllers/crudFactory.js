// Factory sinh bộ hàm CRUD chuẩn cho một Mongoose model.
// Mọi controller theo Bảng 3.4 tái sử dụng để "thêm / lấy / sửa / xóa" mà không lặp code.
// Phản hồi lỗi đẩy về errorHandler (server.js) -> 500 ERROR như mô tả báo cáo (Hình 3.26).

module.exports = function crudFactory(Model) {
  return {
    // GET /<resource>  — lấy tất cả (cho phép lọc qua query, vd ?nodeId=nk1)
    async getAll(req, res, next) {
      try {
        const docs = await Model.find(req.query);
        res.json(docs); // toJSON tự map _id -> id
      } catch (err) {
        next(err);
      }
    },

    // GET /<resource>/:id  — lấy một bản ghi
    async getOne(req, res, next) {
      try {
        const doc = await Model.findById(req.params.id);
        if (!doc) return res.status(404).json({ message: "Không tìm thấy" });
        res.json(doc);
      } catch (err) {
        next(err);
      }
    },

    // POST /<resource>  — thêm mới (cho phép client gửi kèm id tùy chọn)
    async create(req, res, next) {
      try {
        const body = { ...req.body };
        if (body.id && !body._id) {
          body._id = body.id;
          delete body.id;
        }
        const doc = await Model.create(body);
        res.status(201).json(doc);
      } catch (err) {
        next(err);
      }
    },

    // PATCH /<resource>/:id  — cập nhật một phần
    async update(req, res, next) {
      try {
        const body = { ...req.body };
        delete body.id;
        delete body._id;
        const doc = await Model.findByIdAndUpdate(req.params.id, body, {
          new: true,
          runValidators: true,
        });
        if (!doc) return res.status(404).json({ message: "Không tìm thấy" });
        res.json(doc);
      } catch (err) {
        next(err);
      }
    },

    // DELETE /<resource>/:id  — xóa
    async remove(req, res, next) {
      try {
        const doc = await Model.findByIdAndDelete(req.params.id);
        if (!doc) return res.status(404).json({ message: "Không tìm thấy" });
        res.json({ message: "Đã xóa", id: req.params.id });
      } catch (err) {
        next(err);
      }
    },
  };
};
