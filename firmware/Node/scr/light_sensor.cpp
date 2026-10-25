#include "light_sensor.h"
#include "common.h"

LightSensor::LightSensor(uint8_t pin)
    : mPin(pin), mLightValue(0) {}

uint32_t LightSensor::initialize() {
    pinMode(mPin, INPUT);
    return RESULT_OK;
}

bool LightSensor::checkData() {
    float analogValue = analogRead(mPin);
    float lux;
    // Serial.print("Analog value: ");
    // Serial.println(analogValue);
    if (analogValue > 0) {
        lux = ((2702.7 - analogValue) * 10.0) / analogValue;
    } else {
        lux = 0.0;
    }

    if (lux < 0) {
        lux = 0;
    }

    if (lux > 10000) {
        lux = 10000;
    }

    mLightValue = lux;

    return true;
}

uint32_t LightSensor::getData(SensorBlob& data) {

    if (!checkData()) {
        return RESULT_ERR;
    }

    data.has_light = true;
    data.light = mLightValue;

    return RESULT_OK;
}