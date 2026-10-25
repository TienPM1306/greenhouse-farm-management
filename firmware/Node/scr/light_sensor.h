#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "i_sensor_base.h"
#include <Arduino.h>

// #define LIGHT_SENSOR_PIN 34

class LightSensor : public ISensorBase {

public:
    LightSensor(uint8_t pin);

    uint32_t initialize() override;

    uint32_t getData(SensorBlob& data) override;

    bool checkData() override;

private:
    uint8_t mPin;
    float mLightValue;
};

#endif