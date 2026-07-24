#ifndef BHARAT_MSG_WIRE_TYPES_H
#define BHARAT_MSG_WIRE_TYPES_H

#include <stdint.h>

typedef struct {
    uint64_t cap_id;
    uint32_t rights;
    uint32_t object_type;
} bharat_cap_wire_t;

#endif // BHARAT_MSG_WIRE_TYPES_H
