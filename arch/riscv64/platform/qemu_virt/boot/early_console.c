#include "boot/platform_boot_info.h"

// Platform-specific early boot overrides for riscv64 qemu_virt machine
void platform_riscv64_qemu_virt_early_init(platform_boot_info_t *plat) {
    if (!plat) return;

    plat->has_early_console = true;
    plat->uart_phys_base = 0x10000000; // QEMU riscv virt UART base
    plat->uart_type = 16550;

    // QEMU specific reserved regions, etc.
}
