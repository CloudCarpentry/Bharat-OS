#ifndef BHARAT_CONSOLE_DISCOVERY_H
#define BHARAT_CONSOLE_DISCOVERY_H

#include "console_types.h"
#include "console_caps.h"
#include <stddef.h>

/*
 * Console Hardware Capabilities Profile
 */
typedef struct console_hw_profile {
    uint64_t arch_caps;
    uint64_t board_caps;
    uint64_t console_caps;
    uint32_t preferred_backend_mask;
    uint32_t fallback_backend_mask;
} console_hw_profile_t;

/*
 * Board Device Descriptor
 */
typedef struct console_device_desc {
    console_backend_type_t type;
    const char* name;
    uintptr_t base;
    uint32_t irq;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    uint64_t hw_caps;
    uint64_t console_caps;
    int priority;
    bool early_ok;
    bool panic_ok;
} console_device_desc_t;

/*
 * Architecture Discovery Hooks
 */
size_t arch_console_discover(console_device_desc_t* out, size_t max);
console_device_desc_t* arch_console_get_boot_caps(void);
console_device_desc_t* arch_console_get_firmware_console(void);
void arch_console_memory_barriers_for_io(void);
void arch_console_panic_reinit(void);

#endif // BHARAT_CONSOLE_DISCOVERY_H
