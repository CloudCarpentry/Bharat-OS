#include "boot/platform_boot_info.h"

// Platform-specific early boot overrides for arm64 qemu_virt machine
void platform_arm64_qemu_virt_early_init(platform_boot_info_t *plat) {
    if (!plat) return;

    plat->has_early_console = true;
    plat->uart_phys_base = 0x09000000; // QEMU virt UART PL011 base
    plat->uart_type = BHARAT_EARLY_UART_TYPE_PL011;

    // QEMU specific reserved regions, etc.
}
