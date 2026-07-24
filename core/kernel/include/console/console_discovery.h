#pragma once

#include "console_base_types.h"
#include "console_types.h"
#include "console_caps.h"

typedef struct {
    uint8_t type;
    uint8_t early_ok;
    uint8_t panic_ok;
    uint8_t reserved0;
    uint32_t priority;
    uintptr_t base;
    uint32_t irq;
    console_caps_t caps;
    const char *name;
    void *opaque;
} console_device_desc_t;

size_t console_discover_devices(console_device_desc_t *out, size_t max_count);
