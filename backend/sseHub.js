// sseHub.js — Kênh SSE (Server-Sent Events) để backend đẩy realtime trạng thái
// ACK/timeout của relay xuống trình duyệt, không cần polling.
// EventSource của trình duyệt không gửi được custom header, nên endpoint này
// tự xác thực bằng query param ?token= (JWT), KHÔNG đi qua middleware requireAuth
// chung của /v1 (server.js mount route này trước đó).
const { verifyToken } = require("./lib/jwt");
const clients = new Set();

function sseHandler(req, res) {
  const token = req.query.token;
  if (!token) {
    return res.status(401).end();
  }
  try {
    verifyToken(token);
  } catch {
    return res.status(401).end();
  }
  res.set({
    "Content-Type": "text/event-stream",
    "Cache-Control": "no-cache",
    Connection: "keep-alive",
  });
  res.flushHeaders();
  clients.add(res);
  req.on("close", () => clients.delete(res));
}

function broadcast(payload) {
  const line = `data: ${JSON.stringify(payload)}\n\n`;
  for (const res of clients) res.write(line);
}

module.exports = { sseHandler, broadcast };
