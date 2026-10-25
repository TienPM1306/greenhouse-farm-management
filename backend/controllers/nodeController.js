const Node = require("../models/Node");
const Gateway = require("../models/Gateway");
const crud = require("./crudFactory")(Node);
const { encodeChangeGatewayMac } = require("../lib/encodeNodeDiscovery");
const { publishCommand } = require("../mqttHandler");

async function update(req, res, next) {
  try {
    const body = { ...req.body };
    delete body.id;
    delete body._id;

    const before = await Node.findById(req.params.id);
    if (!before) return res.status(404).json({ message: "Khong tim thay" });

    const adoptingNow =
      body.registered === true &&
      before.registered !== true &&
      before.gatewayId &&
      before.mac;

    if (adoptingNow) {
      const gw = await Gateway.findById(before.gatewayId);
      if (!gw || !gw.mac) {
        return res.status(409).json({ message: "Khong xac dinh duoc gateway/MAC de gan node" });
      }

      const payloadHex = encodeChangeGatewayMac(gw.mac, before.mac);
      await publishCommand(gw.mac, payloadHex);
    }

    const doc = await Node.findByIdAndUpdate(req.params.id, body, {
      new: true,
      runValidators: true,
    });

    res.json(doc);
  } catch (err) {
    next(err);
  }
}

module.exports = { ...crud, update };
