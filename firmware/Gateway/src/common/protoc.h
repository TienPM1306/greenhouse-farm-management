#ifndef PROTO_CODEC_H
#define PROTO_CODEC_H

#include <stdint.h>
#include <string.h>

typedef enum
{
    PROTO_BOOL = 0,
    PROTO_UINT8,
    PROTO_UINT16,
    PROTO_UINT32,
    PROTO_FLOAT
} ProtoType;

typedef struct
{
    uint8_t  field_id;
    ProtoType type;
    uint16_t  offset;
    uint16_t  size;
    uint16_t  has_offset;
} ProtoField;

class ProtoCodec
{
public:
    static uint16_t encode(uint8_t *buffer,
                           uint16_t size,
                           void *message,
                           const ProtoField *fields,
                           uint8_t fieldCount)
    {
        uint16_t index = 0;
        for (uint8_t i = 0; i < fieldCount; i++) {
            bool *has = (bool *)((uint8_t *)message + fields[i].has_offset);
            if (!(*has))
                continue;
            buffer[index++] = fields[i].field_id;
            buffer[index++] = fields[i].type;
            memcpy(&buffer[index],
                   (uint8_t *)message + fields[i].offset,
                   fields[i].size);
            index += fields[i].size;
        }
        return index;
    }

    static void decode(uint8_t *buffer,
                       uint16_t size,
                       void *message,
                       const ProtoField *fields,
                       uint8_t fieldCount)
    {
        uint16_t index = 0;
        while (index + 2 <= size) {
            uint8_t field_id = buffer[index++];
            uint8_t type     = buffer[index++];
            for (uint8_t i = 0; i < fieldCount; i++) {
                if (fields[i].field_id == field_id) {
                    if (index + fields[i].size > size) break;
                    memcpy((uint8_t *)message + fields[i].offset,
                           &buffer[index],
                           fields[i].size);
                    bool *has = (bool *)((uint8_t *)message + fields[i].has_offset);
                    *has = true;
                    index += fields[i].size;
                    break;
                }
            }
        }
    }
};

#endif
