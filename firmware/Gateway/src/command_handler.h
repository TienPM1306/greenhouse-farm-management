#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "common/command.h"
#include "common/message.h"
#include "common/protoc.h"

void handleMqttCommand(uint8_t commandType, const uint8_t *data, unsigned int length);

#endif
