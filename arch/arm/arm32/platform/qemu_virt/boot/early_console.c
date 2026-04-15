#include "boot/platform_boot_info.h"

void platform_arm32_qemu_virt_early_init(platform_boot_info_t *plat) {
    if (!plat) {
        return;
    }

    plat->has_early_console = true;
    plat->uart_phys_base = 0x09000000ULL;
    plat->uart_type = 0x0001; /* PL011 */
}
