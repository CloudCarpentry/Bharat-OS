#include "console/console_discovery.h"
#include <stddef.h>

/* Gather descriptors from all arch-specific discovery helpers */

size_t arch_console_discover(console_device_desc_t *out, size_t max_count);

size_t console_discover_devices(console_device_desc_t *out, size_t max_count) {
    if (!out || max_count == 0) return 0;

    // In a multi-arch unified build this would use linker sets or weak refs.
    // For now, assume arch_console_discover is provided by the active architecture.
    return arch_console_discover(out, max_count);
}
