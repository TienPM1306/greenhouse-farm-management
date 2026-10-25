#include "dht_sensor.h"
#include "common.h"

DHTSensor::DHTSensor(uint8_t pin)
    : mPin(pin),
      mTemperature(0),
      mHumidity(0)
{
}

uint32_t DHTSensor::initialize()
{
    // DHTesp dùng cơ chế đọc an toàn cho ESP32 (không treo Interrupt WDT)
    mDht.setup(mPin, DHTesp::DHT11);
    Serial.println(F("DHT Initialized!"));
    return RESULT_OK;
}

bool DHTSensor::checkData()
{
    TempAndHumidity th = mDht.getTempAndHumidity();

    if (mDht.getStatus() != DHTesp::ERROR_NONE ||
        isnan(th.temperature) || isnan(th.humidity))
    {
        Serial.print(F("DHT read failed: "));
        Serial.println(mDht.getStatusString());
        return false;
    }

    mTemperature = th.temperature;
    mHumidity = th.humidity;

    return true;
}

uint32_t DHTSensor::getData(SensorBlob& data)
{
    if (!checkData())
    {
        return RESULT_ERR;
    }

    data.has_temperature = true;
    data.temperature = mTemperature;

    data.has_humidity = true;
    data.humidity = mHumidity;

    return RESULT_OK;
}
