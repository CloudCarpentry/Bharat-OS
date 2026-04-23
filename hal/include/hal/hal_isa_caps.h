#pragma once

#include <stdbool.h>
#include <stdint.h>

struct hal_isa_caps {
    const char *isa_name;
    bool has_mmu;
    bool has_mpu;
    uint32_t min_alignment;
};

const struct hal_isa_caps *hal_query_isa_caps(void);
