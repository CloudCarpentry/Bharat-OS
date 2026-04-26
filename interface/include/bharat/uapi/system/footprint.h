/*
 * Transitional UAPI placeholder.
 * This header exists to unblock current build wiring.
 */
#ifndef BHARAT_UAPI_SYSTEM_FOOTPRINT_H
#define BHARAT_UAPI_SYSTEM_FOOTPRINT_H

#include <stdint.h>

struct bharat_footprint_stats {
    uint64_t code_bytes;
    uint64_t data_bytes;
    uint64_t heap_bytes;
    uint64_t stack_bytes;
};

typedef struct bharat_footprint_stats bharat_kernel_footprint_t;

#endif // BHARAT_UAPI_SYSTEM_FOOTPRINT_H
