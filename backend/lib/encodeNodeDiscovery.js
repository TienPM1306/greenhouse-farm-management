const CMD_CHANGE_GATEWAY_MAC = 0x02;
const CMD_SCAN_REQUEST = 0x06;

function normalizeMac(value, label) {
  const mac = String(value || "").toUpperCase();
  if (!/^[0-9A-F]{12}$/.test(mac)) {
    throw new Error(`${label} không hợp lệ (cần 12 hex)`);
  }
  return mac;
}

function encodeScanRequest() {
  return Buffer.from([CMD_SCAN_REQUEST]).toString("hex").toUpperCase();
}

function encodeChangeGatewayMac(gatewayMac, nodeMac) {
  const gw = Buffer.from(normalizeMac(gatewayMac, "gatewayMac"), "hex");
  const node = Buffer.from(normalizeMac(nodeMac, "nodeMac"), "hex");

  const packet = Buffer.concat([
    Buffer.from([CMD_CHANGE_GATEWAY_MAC]),
    Buffer.from([0x01, 0x01]),
    gw,
    Buffer.from([0x02, 0x01]),
    node,
  ]);
  return packet.toString("hex").toUpperCase();
}

module.exports = {
  CMD_CHANGE_GATEWAY_MAC,
  CMD_SCAN_REQUEST,
  encodeScanRequest,
  encodeChangeGatewayMac,
};
