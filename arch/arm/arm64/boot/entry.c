#include "bharat/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/hal_boot.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Early boot entry for ARM64
void kernel_main(uintptr_t fdt_ptr) {
    boot_info_t boot = {0};

    hal_serial_init();
    hal_serial_write("!ARM64 Boot Started\n");
    hal_serial_write("!FDT ptr initial: ");
    hal_serial_write_hex(fdt_ptr);
    hal_serial_write("\n");

    if (fdt_ptr == 0) {
        // Probe common locations for QEMU virt
        fdt_ptr = 0x40000000;
        hal_serial_write("!Probing FDT at fallback: ");
        hal_serial_write_hex(fdt_ptr);
        hal_serial_write("\n");
    }

    if (fdt_ptr == 0 || !fdt_is_valid((void*)fdt_ptr)) {
        // Probe common locations for QEMU virt
        fdt_ptr = 0x40000000;
        hal_serial_write("!Probing FDT at fallback: ");
        hal_serial_write_hex(fdt_ptr);
        hal_serial_write("\n");
        if (!fdt_is_valid((void*)fdt_ptr)) {
            kernel_panic("FDT not provided by bootloader and fallback failed");
        }
    }

    extern void arm_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    arm_fdt_parse_common(&boot, (const void*)fdt_ptr);

    // Pass the normalized boot contract
    kernel_main_common(&boot);
}
