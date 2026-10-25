// Seed backend/db.json cho proj_test2 — Hệ thống Quản lý Nông trại Nhà kính (DATN).
// Schema khớp FinalProject/Website/db.json; khác biệt: node = nhà kính (2 NK),
// mỗi NK 3 cảm biến (nhiệt độ/độ ẩm/ánh sáng) + 3 thiết bị (bơm/quạt/đèn).
// Chạy: node scripts/seed.js   (ghi ra ../db.json — thực ra ghi backend/db.json)

const fs = require("fs");
const path = require("path");

// sensorType: 1=Nhiệt độ(°C), 2=Độ ẩm(%), 3=Ánh sáng(Lux) — khớp FinalProject
const SENSOR_TYPES = [1, 2, 3];

// Sinh mảng điểm dữ liệu {time, value} quanh một giá trị nền + dao động
function points(baseDay, count, base, swing) {
  const out = [];
  for (let i = 0; i < count; i++) {
    const t = new Date(baseDay);
    t.setUTCHours(6 + i * Math.floor(12 / count), (i * 17) % 60, 0, 0);
    const value = +(base + (Math.sin(i * 1.3) * swing)).toFixed(2);
    out.push({ time: t.toISOString(), value });
  }
  return out;
}

// Dải giá trị mẫu theo loại cảm biến
function rangeFor(type) {
  if (type === 1) return { base: 28, swing: 4 };    // nhiệt độ °C
  if (type === 2) return { base: 70, swing: 8 };    // độ ẩm %
  return { base: 8000, swing: 3000 };               // ánh sáng Lux
}

function sensorData() {
  const today = "2025-06-18";
  const { base, swing } = this;
  return {
    today: {
      dataMinute: points(today, 4, base, swing),
      dataHour: points(today, 2, base, swing),
    },
    pastDay: ["2025-06-16", "2025-06-17"].map((date) => ({
      date,
      dataMinute: points(date, 3, base, swing),
      dataHour: points(date, 2, base, swing),
    })),
  };
}

const gatewayId = "gw01";

// 2 nhà kính = 2 node
const nodes = [
  {
    id: "nk1",
    gatewayId,
    name: "Nhà kính 1",
    nodeAddh: "1",
    nodeAddl: "1",
    nodeDesc: "Nhà kính số 1",
    nodePosition: "Khu A",
    nodeCreatedDay: "2025-06-18",
    nodeAddress: "10.83567377888011,106.72191238350933",
  },
  {
    id: "nk2",
    gatewayId,
    name: "Nhà kính 2",
    nodeAddh: "2",
    nodeAddl: "2",
    nodeDesc: "Nhà kính số 2",
    nodePosition: "Khu B",
    nodeCreatedDay: "2025-06-18",
    nodeAddress: "10.842754925426085,106.81272125191755",
  },
];

// 3 thiết bị / nhà kính: máy bơm(pin1), quạt(pin2), đèn(pin3)
const DEVICE_DEFS = [
  { name: "Máy bơm", pin: "1", status: false },
  { name: "Quạt", pin: "2", status: false },
  { name: "Đèn", pin: "3", status: false },
];

const sensors = [];
const devices = [];
const conditions = [];
const schedules = [];

nodes.forEach((node) => {
  // cảm biến
  SENSOR_TYPES.forEach((type, i) => {
    const r = rangeFor(type);
    sensors.push({
      id: `${node.id}-s${type}`,
      gatewayId,
      nodeId: node.id,
      sensorType: type,
      data: sensorData.call(r),
    });
  });
  // thiết bị
  DEVICE_DEFS.forEach((d) => {
    devices.push({
      id: `${node.id}-d${d.pin}`,
      gatewayId,
      nodeId: node.id,
      name: `${d.name} ${node.name}`,
      pin: d.pin,
      description: `${d.name} của ${node.name}`,
      status: d.status,
    });
  });
});

// Điều kiện tự động hóa — logic nông nghiệp cơ bản (cho mỗi nhà kính)
nodes.forEach((node) => {
  // Nhiệt độ cao -> bật quạt
  conditions.push({
    id: `${node.id}-c-fan`,
    gatewayId,
    nodeId: node.id,
    deviceName: `Quạt ${node.name}`,
    devicePin: "2",
    status: true,
    sensorType: 1, // nhiệt độ
    minValue: 32,
    maxValue: 50,
  });
  // Độ ẩm thấp -> bật máy bơm
  conditions.push({
    id: `${node.id}-c-pump`,
    gatewayId,
    nodeId: node.id,
    deviceName: `Máy bơm ${node.name}`,
    devicePin: "1",
    status: true,
    sensorType: 2, // độ ẩm
    minValue: 0,
    maxValue: 50,
  });
  // Ánh sáng yếu -> bật đèn
  conditions.push({
    id: `${node.id}-c-light`,
    gatewayId,
    nodeId: node.id,
    deviceName: `Đèn ${node.name}`,
    devicePin: "3",
    status: true,
    sensorType: 3, // ánh sáng
    minValue: 0,
    maxValue: 3000,
  });
});

// Vài lịch hẹn giờ mẫu: bật đèn buổi tối ở mỗi nhà kính
nodes.forEach((node) => {
  schedules.push({
    id: `${node.id}-sch-light`,
    gatewayId,
    nodeId: node.id,
    deviceName: `Đèn ${node.name}`,
    devicePin: "3",
    status: true,
    startTime: "2025-06-18T11:00:00.000Z",
    endTime: "2025-06-18T13:00:00.000Z",
    dailyRepeat: true,
  });
});

const db = {
  users: [
    {
      id: "user1",
      name: "Quản trị viên",
      birth: "2000-01-01",
      phone: "0900000000",
      email: "admin@greenhouse.local",
      username: "admin",
      password: "admin123",
      avatar: "",
      home: "",
    },
  ],
  gateways: [
    {
      id: gatewayId,
      userId: "user1",
      projectName: "Nông trại nhà kính",
      gatewayAddress: "Thủ Đức, TP.HCM",
      gatewayName: "Gateway1",
      gatewayDesc: "Gateway gom dữ liệu 2 nhà kính",
      gatewayCreatedDay: "2025-06-18",
    },
  ],
  nodes,
  sensors,
  devices,
  schedules,
  conditions,
  notifies: [
    {
      id: "ntf1",
      gatewayId,
      nodeId: "nk1",
      type: "warning",
      title: "Nhiệt độ vượt ngưỡng",
      message: "Nhà kính 1: nhiệt độ 35°C vượt ngưỡng 32°C, đã bật quạt.",
      createdAt: "2025-06-18T05:10:00.000Z",
    },
  ],
};

const outPath = path.join(__dirname, "..", "db.json");
fs.writeFileSync(outPath, JSON.stringify(db, null, 2), "utf8");
console.log("Đã ghi", outPath);
console.log(
  `users:${db.users.length} gateways:${db.gateways.length} nodes:${db.nodes.length} ` +
    `sensors:${db.sensors.length} devices:${db.devices.length} ` +
    `schedules:${db.schedules.length} conditions:${db.conditions.length} notifies:${db.notifies.length}`
);
