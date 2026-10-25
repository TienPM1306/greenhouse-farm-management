// ISensorBase.h

#ifndef ISENSOR_BASE_H
#define ISENSOR_BASE_H

#include <stdint.h>
#include "message.h"

class ISensorBase {
public:
    virtual ~ISensorBase() {}

    virtual uint32_t initialize() = 0;

    virtual uint32_t getData(SensorBlob& data) = 0;

    virtual bool checkData() = 0;
};

#endif