#ifndef BHARAT_UAPI_DEVICE_ACTUATOR_H
#define BHARAT_UAPI_DEVICE_ACTUATOR_H

#include <stddef.h>
#include <stdint.h>

#define BH_ACTUATOR_ABI_VERSION 1u

typedef uint32_t bh_actuator_handle_t;

typedef enum bh_actuator_type {
    BH_ACTUATOR_MOTOR = 1
} bh_actuator_type_t;

enum {
    BH_ACTUATOR_COMMAND_NORMALIZED = 1u << 0,
    BH_ACTUATOR_COMMAND_ARM = 1u << 1,
    BH_ACTUATOR_COMMAND_DISARM = 1u << 2
};

typedef struct bh_actuator_command {
    uint32_t abi_version;
    uint32_t flags;
    uint64_t request_id;
    float value;
    uint32_t reserved;
} bh_actuator_command_t;

_Static_assert(sizeof(bh_actuator_command_t) == 24u, "actuator command ABI size");
_Static_assert(offsetof(bh_actuator_command_t, request_id) == 8u, "actuator request ABI offset");

#endif
