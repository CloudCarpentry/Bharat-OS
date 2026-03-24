#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/hal_boot.h"
#include "hal/riscv_bsp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Early boot entry for RISC-V 64
void kernel_main(uint64_t hart_id, uintptr_t fdt_ptr) {
    boot_info_t boot = {0};
    boot.boot_cpu_id = hart_id;

    hal_serial_init();
    hal_serial_write("!RISCV64 Boot Started\n");
    hal_serial_write("!FDT ptr initial: ");
    hal_serial_write_hex(fdt_ptr);
    hal_serial_write("\n");

    if (fdt_ptr == 0 || !fdt_is_valid((void*)fdt_ptr)) {
        /* Fallback for QEMU virt: FDT at end of 256MB RAM */
        fdt_ptr = 0x8fe00000;
        hal_serial_write("!Probing FDT at fallback: ");
        hal_serial_write_hex(fdt_ptr);
        hal_serial_write("\n");
        if (!fdt_is_valid((void*)fdt_ptr)) {
            kernel_panic("FDT not provided by bootloader and fallback failed");
        }
    }

    hal_riscv_set_boot_info(hart_id, (uint64_t)fdt_ptr);

    extern void riscv_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    riscv_fdt_parse_common(&boot, (const void*)fdt_ptr);

    kernel_main_common(&boot);
}
