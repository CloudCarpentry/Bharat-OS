#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/hal_boot.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uintptr_t arm64_find_fdt_fallback(uintptr_t start, uintptr_t end, uintptr_t step) {
    for (uintptr_t p = start; p < end; p += step) {
        if (fdt_is_valid((void*)p)) {
            return p;
        }
    }
    return 0;
}

// Early boot entry for ARM64
void kernel_main(uintptr_t fdt_ptr) {
    boot_info_t boot;
    boot_info_init(&boot);

    hal_serial_init();
    hal_serial_write("ARM64 Boot Started\n");
    hal_serial_write("FDT ptr initial: ");
    hal_serial_write_hex(fdt_ptr);
    hal_serial_write("\n");

    if (fdt_ptr == 0 || !fdt_is_valid((void*)fdt_ptr)) {
        // Probe common locations for QEMU virt.
        // Some host/QEMU combinations may fail to preserve x0 handoff.
        const uintptr_t fdt_scan_start = 0x40000000ULL;
        const uintptr_t fdt_scan_end = 0x50000000ULL;
        const uintptr_t fdt_scan_step = 0x1000ULL;

        hal_serial_write("Probing FDT fallback range: ");
        hal_serial_write_hex(fdt_scan_start);
        hal_serial_write("..");
        hal_serial_write_hex(fdt_scan_end);
        hal_serial_write("\n");

        uintptr_t discovered = arm64_find_fdt_fallback(fdt_scan_start, fdt_scan_end, fdt_scan_step);
        if (discovered == 0) {
            kernel_panic("FDT not provided by bootloader and fallback scan failed");
        }

        fdt_ptr = discovered;
        hal_serial_write("FDT fallback discovered at: ");
        hal_serial_write_hex(fdt_ptr);
        hal_serial_write("\n");
    }

    extern void arm_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    arm_fdt_parse_common(&boot, (const void*)fdt_ptr);

    // Pass the normalized boot contract
    kernel_main_common(&boot);
}
