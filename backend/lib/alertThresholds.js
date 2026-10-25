// alertThresholds.js — ngưỡng cảnh báo an toàn nhà kính (cố định). Thuần, không phụ thuộc
// DB/MQTT. sensorType: 1=nhiệt độ, 2=độ ẩm, 3=ánh sáng. max=null → không kiểm cận trên.
const THRESHOLDS = {
  1: { label: "Nhiệt độ", unit: "°C", min: 18, max: 35 },
  2: { label: "Độ ẩm", unit: "%", min: 40, max: 85 },
  3: { label: "Ánh sáng", unit: "", min: 0.5, max: null },
};

// So sánh value với ngưỡng của sensorType.
// Trả { breached, kind:"low"|"high"|null, label, unit, min, max }.
function evaluate(sensorType, value) {
  const t = THRESHOLDS[sensorType];
  if (!t || typeof value !== "number" || Number.isNaN(value)) {
    return { breached: false, kind: null, label: "", unit: "", min: null, max: null };
  }
  let kind = null;
  if (value < t.min) kind = "low";
  else if (t.max != null && value > t.max) kind = "high";
  return { breached: kind !== null, kind, label: t.label, unit: t.unit, min: t.min, max: t.max };
}

module.exports = { THRESHOLDS, evaluate };
