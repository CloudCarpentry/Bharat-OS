#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include <stdint.h>
#include <stddef.h>

// Early boot entry for ARM32
void kernel_main(uintptr_t fdt_ptr) {
    boot_info_t boot;
    boot_info_init(&boot);

    // Abstracted FDT validation & parsing logic
    if (fdt_ptr == 0 || !fdt_is_valid((void*)fdt_ptr)) {
        // Fallback
        fdt_ptr = 0x40000000;
    }

    extern void arm_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    if (fdt_is_valid((void*)fdt_ptr)) {
        arm_fdt_parse_common(&boot, (const void*)fdt_ptr);
    } else {
        kernel_panic("FDT NOT FOUND AT FALLBACK!");
    }

    // Pass the normalized boot contract
    kernel_main_common(&boot);
}
