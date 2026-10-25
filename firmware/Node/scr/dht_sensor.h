#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "i_sensor_base.h"
#include <Arduino.h>
#include <DHTesp.h>

// #define DHT_SENSOR_PIN 27

class DHTSensor : public ISensorBase {

public:
    DHTSensor(uint8_t pin);

    uint32_t initialize() override;

    uint32_t getData(SensorBlob& data) override;

    bool checkData() override;

private:
    uint8_t mPin;
    DHTesp mDht;

    float mTemperature;
    float mHumidity;
};

#endif
