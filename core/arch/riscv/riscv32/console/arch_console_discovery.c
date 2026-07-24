#include "console/console_discovery.h"

size_t arch_console_discover(console_device_desc_t *out, size_t max_count) {
    (void)out;
    (void)max_count;
    /*
     * Intentionally non-authoritative.
     * Boot console selection is resolved via platform_get_boot_console_desc()
     * and consumed by kernel console discovery.
     */
    return 0;
}
