// ─────────────────────────────────────────────────────────────────────────────
// mqttObserve.js — GIAI ĐOẠN 3a (OBSERVE). CHỈ ĐỌC, không ghi DB, không publish.
// Mục tiêu: subscribe broker để xem dữ liệu gateway phát lên + PHÁT HIỆN nodeMac thật.
// An toàn tuyệt đối với phần cứng LIVE (không gửi lệnh điều khiển nào).
// Chạy: npm run observe   (hoặc: node mqttObserve.js)
// Dừng: Ctrl + C
// ─────────────────────────────────────────────────────────────────────────────
require("dotenv").config();
const mqtt = require("mqtt");

const HOST = process.env.MQTT_HOST;
const PORT = Number(process.env.MQTT_PORT || 8883);
const USERNAME = process.env.MQTT_USERNAME;
const PASSWORD = process.env.MQTT_PASSWORD;

if (!HOST || !USERNAME) {
  console.error("✗ Thiếu cấu hình MQTT trong .env (MQTT_HOST / MQTT_USERNAME / MQTT_PASSWORD).");
  process.exit(1);
}

// Topic uplink theo firmware (định danh bằng MAC gateway). Dùng wildcard '+' để bắt mọi gateway.
const TOPICS = ["+/node_data", "+/node_response", "+/online", "agriculture/abc123@/newgateway"];

// Bảng theo dõi MAC đã thấy (để điền mapping cho GĐ 3b)
const seenGateways = new Set(); // gwMAC (từ topic)
const seenNodes = new Set(); // nodeMac (từ payload JSON)

const ts = () => new Date().toISOString().replace("T", " ").slice(0, 19);

console.log(`→ Đang kết nối broker mqtts://${HOST}:${PORT} (user: ${USERNAME})`);
const client = mqtt.connect(`mqtts://${HOST}:${PORT}`, {
  username: USERNAME,
  password: PASSWORD,
  connectTimeout: 8000,
});

client.on("connect", () => {
  console.log("✓ Kết nối broker THÀNH CÔNG. Đang subscribe...");
  client.subscribe(TOPICS, (err) => {
    if (err) {
      console.error("✗ Subscribe lỗi:", err.message);
      process.exit(1);
    }
    TOPICS.forEach((t) => console.log("  ✓ subscribed:", t));
    console.log("\n── Đang chờ message (bật gateway lên là thấy). Ctrl+C để dừng. ──\n");
  });
});

client.on("message", (topic, buf) => {
  const gwMac = topic.split("/")[0]; // MAC gateway nằm ở cấp đầu topic
  const raw = buf.toString();
  const kind = topic.split("/").slice(1).join("/") || topic;

  if (gwMac && gwMac.length === 12) seenGateways.add(gwMac);

  let extra = "";
  // node_data / node_response là JSON -> parse để lấy nodeMac
  try {
    const obj = JSON.parse(raw);
    if (obj.nodeMac) {
      seenNodes.add(obj.nodeMac);
      extra = `  → nodeMac=${obj.nodeMac}`;
    }
  } catch {
    // online ("1") hoặc payload nhị phân -> in dạng hex cho dễ nhìn
    if (kind !== "online") extra = "  (nhị phân/không-JSON)";
  }

  console.log(`[${ts()}] topic=${topic}`);
  console.log(`            gwMAC=${gwMac}  loại=${kind}${extra}`);
  console.log(`            payload=${raw}`);
  console.log(
    `            ↳ Đã thấy: ${seenGateways.size} gateway, ${seenNodes.size} node` +
      (seenNodes.size ? ` [${[...seenNodes].join(", ")}]` : "")
  );
  console.log("");
});

client.on("error", (err) => console.error("✗ Lỗi MQTT:", err.message));

// In tổng kết khi dừng
process.on("SIGINT", () => {
  console.log("\n── TỔNG KẾT (để điền mapping GĐ 3b) ──");
  console.log("Gateway MAC đã thấy:", [...seenGateways].join(", ") || "(chưa có)");
  console.log("Node MAC đã thấy   :", [...seenNodes].join(", ") || "(chưa có)");
  client.end(true, () => process.exit(0));
});
