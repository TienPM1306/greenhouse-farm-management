#ifndef LORA_SERVICE_H
#define LORA_SERVICE_H

#include <Arduino.h>
#include <LoRa_E32.h>

#define LORA_RXD2   16
#define LORA_TXD2   17
#define LORA_PIN_M0 4
#define LORA_PIN_M1 2
#define LORA_PIN_AUX 15

typedef struct
{
    AIR_DATA_RATE                   airDataRate;
    UART_BPS_TYPE                   baudRate;
    E32_UART_PARITY                 parityType;
    FIDEX_TRANSMISSION              transmitType;
    FORWARD_ERROR_CORRECTION_SWITCH fec;
    WIRELESS_WAKE_UP_TIME           wakeUpTime;
    TRANSMISSION_POWER              power;
} LoraConfig;

class LoraService
{
public:
    static LoraService *getInstance();

    uint32_t initialize(LoraConfig *config);

    uint32_t send(const void *data, uint16_t size);
    uint32_t receive(uint8_t *dataReceive, uint16_t *size);

    bool connected();
    void printParameters();

private:
    LoraService();

    static LoraService *mInstance;

    HardwareSerial *mSerial;
    LoRa_E32       *mLora;
    Configuration   mConfiguration;
    ResponseStatus  mSendCheck;
    ResponseContainer mReceiveCheck;
    bool mConnected;

    // readString() của thư viện E32 đọc TOÀN BỘ byte đang có trong UART buffer
    // trong 1 lần gọi — nếu 2 gói LoRa đến gần nhau, chúng có thể dính làm 1
    // chuỗi. mRxQueue giữ phần dư (gói thứ 2, 3...) sau khi tách gói đầu theo
    // length-prefix, để lần receive() kế tiếp trả ra ngay không cần đọc UART.
    static constexpr uint16_t RX_QUEUE_SIZE = 256;
    uint8_t  mRxQueue[RX_QUEUE_SIZE];
    uint16_t mRxQueueLen = 0;

    uint32_t popFromQueue(uint8_t *dataReceive, uint16_t *size);
};

#endif
