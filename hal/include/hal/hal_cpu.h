#pragma once

#include <stdint.h>

struct hal_cpu_info {
    uint32_t logical_id;
    uint32_t cluster_id;
    uint32_t flags;
};

int hal_cpu_current(void);
const struct hal_cpu_info *hal_cpu_info(uint32_t cpu_id);
