#include "boot/platform_boot_info.h"

// Platform-specific early boot overrides for x86_64 q35 machine
void platform_q35_early_init(platform_boot_info_t *plat) {
    if (!plat) return;

    // e.g. hardcode early serial or specific memory quirks
    plat->has_early_console = true;
    plat->uart_phys_base = 0x3F8; // COM1
    plat->uart_type = BHARAT_EARLY_UART_TYPE_NS16550;

    // QEMU specific reserved regions, etc.
}
