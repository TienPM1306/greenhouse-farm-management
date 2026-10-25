#include "lora_service.h"
#include "common.h"

LoraService *LoraService::mInstance = nullptr;

LoraService *LoraService::getInstance()
{
    if (mInstance == nullptr) {
        mInstance = new LoraService();
    }
    return mInstance;
}

LoraService::LoraService()
{
    mSerial    = new HardwareSerial(2);
    mLora      = new LoRa_E32(mSerial, LORA_PIN_AUX, LORA_PIN_M0, LORA_PIN_M1);
    mConnected = false;
}

uint32_t LoraService::initialize(LoraConfig *config)
{
    mLora->begin();

    ResponseStructContainer c = mLora->getConfiguration();
    mConfiguration = *(Configuration *)c.data;

    mConfiguration.ADDH = 0xFF;
    mConfiguration.ADDL = 0xFF;
    mConfiguration.CHAN  = 0x17;

    mConfiguration.SPED.airDataRate  = config->airDataRate;
    mConfiguration.SPED.uartBaudRate = config->baudRate;
    mConfiguration.SPED.uartParity   = config->parityType;

    mConfiguration.OPTION.fixedTransmission  = config->transmitType;
    mConfiguration.OPTION.fec                = config->fec;
    mConfiguration.OPTION.wirelessWakeupTime = config->wakeUpTime;
    mConfiguration.OPTION.transmissionPower  = config->power;

    if (mLora->setConfiguration(mConfiguration, WRITE_CFG_PWR_DWN_SAVE).code != E32_SUCCESS) {
        Serial.println(F("[LoRa] Setup error"));
        mConnected = false;
        return RESULT_ERR;
    }

    mLora->setMode(MODE_0_NORMAL);
    printParameters();
    mConnected = true;
    return RESULT_OK;
}

uint32_t LoraService::send(const void *data, uint16_t size)
{
    // Length-prefix (1 byte) trước payload thật: bên nhận dùng nó để tách
    // đúng ranh giới gói khi nhiều gói LoRa đến dính nhau trong 1 lần đọc
    // UART (readString() đọc hết buffer, không tự phân khung).
    if (size == 0 || size > 254) {
        Serial.println(F("[LoRa] Send error: invalid size for length-prefix"));
        return RESULT_ERR;
    }
    uint8_t framed[1 + 254];
    framed[0] = (uint8_t)size;
    memcpy(framed + 1, data, size);

    mSendCheck = mLora->sendMessage(framed, size + 1);
    if (mSendCheck.getResponseDescription() != F("Success")) {
        Serial.println(F("[LoRa] Send error"));
        return RESULT_ERR;
    }
    return RESULT_OK;
}

uint32_t LoraService::popFromQueue(uint8_t *dataReceive, uint16_t *size)
{
    if (mRxQueueLen == 0) return RESULT_NO_DATA;

    uint16_t payloadLen = mRxQueue[0];
    if (payloadLen == 0 || (uint16_t)(1 + payloadLen) > mRxQueueLen) {
        // Khung hỏng (length-prefix không khớp dữ liệu còn lại) -> bỏ hàng đợi.
        Serial.println(F("[LoRa] Queue: framing rách, xóa hàng đợi"));
        mRxQueueLen = 0;
        return RESULT_ERR;
    }

    uint16_t maxSize = *size;
    if (payloadLen > maxSize) {
        Serial.printf("[LoRa] Queue: payload %d vuot buffer %d\n", payloadLen, maxSize);
        mRxQueueLen = 0;
        return RESULT_ERR;
    }

    memcpy(dataReceive, mRxQueue + 1, payloadLen);
    *size = payloadLen;

    uint16_t consumed = 1 + payloadLen;
    memmove(mRxQueue, mRxQueue + consumed, mRxQueueLen - consumed);
    mRxQueueLen -= consumed;

    return RESULT_OK;
}

uint32_t LoraService::receive(uint8_t *dataReceive, uint16_t *size)
{
    // Trả gói còn tồn trong hàng đợi trước (gói dính từ lần đọc UART trước).
    if (mRxQueueLen > 0) {
        return popFromQueue(dataReceive, size);
    }

    if (!mLora->available()) {
        return RESULT_NO_DATA;
    }

    mReceiveCheck = mLora->receiveMessage();

    if (mReceiveCheck.status.getResponseDescription() != F("Success")) {
        Serial.println(F("[LoRa] Receive status failed"));
        return RESULT_ERR;
    }

    uint16_t rawLen = static_cast<uint16_t>(mReceiveCheck.data.length());
    if (rawLen == 0) {
        return RESULT_ERR;
    }
    if (rawLen > RX_QUEUE_SIZE) {
        Serial.printf("[LoRa] Raw data %d vuot RX_QUEUE_SIZE, cat bot\n", rawLen);
        rawLen = RX_QUEUE_SIZE;
    }

    memcpy(mRxQueue, mReceiveCheck.data.c_str(), rawLen);
    mRxQueueLen = rawLen;

    Serial.printf("[LoRa] Received %d raw bytes (UART)\n", rawLen);
    return popFromQueue(dataReceive, size);
}

bool LoraService::connected()
{
    return mConnected;
}

void LoraService::printParameters()
{
    Serial.println(F("------ LoRa Config ------"));
    Serial.printf("Channel: %d\n", mConfiguration.CHAN);
    Serial.print(F("Air Rate: "));
    Serial.println(mConfiguration.SPED.getAirDataRate());
    Serial.print(F("Baud: "));
    Serial.println(mConfiguration.SPED.getUARTBaudRate());
    Serial.print(F("Power: "));
    Serial.println(mConfiguration.OPTION.getTransmissionPowerDescription());
    Serial.println(F("-------------------------"));
}
