#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include <stdint.h>
#include <stddef.h>

extern void hal_serial_write(const char *s);
extern void hal_serial_init(void);

// Early boot entry for ARM32
void kernel_main(uintptr_t fdt_ptr) {
    /* hal_serial_write falls back to bare PL011 write (0x09000000) when
     * early_console is not yet bound, so the boot marker is safe to emit
     * before hal_serial_init(). The UART itself was configured in boot.S. */
    hal_serial_write("BOOT: kernel_main reached\n");

    /* Now bind the proper early console driver for all subsequent output */
    hal_serial_init();

    static boot_info_t boot;
    boot_info_init(&boot);
    boot.arch = BOOT_ARCH_ARM32;

    if (fdt_ptr != 0 && fdt_is_valid((void*)fdt_ptr)) {
        extern void arm_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
        arm_fdt_parse_common(&boot, (const void*)fdt_ptr);
    }

    // Pass the normalized boot contract
    kernel_main_common(&boot);
}
