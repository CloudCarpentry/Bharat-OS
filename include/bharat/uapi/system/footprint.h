#pragma once

#include <stdint.h>

struct bharat_footprint_stats {
    uint64_t static_bytes;
    uint64_t heap_bytes;
    uint64_t per_core_bytes;
    uint64_t driver_bytes;
};

int bharat_footprint_read(struct bharat_footprint_stats *out);
