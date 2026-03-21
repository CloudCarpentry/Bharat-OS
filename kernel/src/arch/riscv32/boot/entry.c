#include "bharat/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/riscv_bsp.h"
#include <stdint.h>
#include <stddef.h>

// Early boot entry for RISC-V 32
void kernel_main(uint64_t hart_id, uintptr_t fdt_ptr) {
    boot_info_t boot = {0};
    boot.boot_cpu_id = hart_id;

    hal_riscv_set_boot_info(hart_id, (uint64_t)fdt_ptr);

    if (fdt_ptr == 0 || !fdt_is_valid((void*)fdt_ptr)) {
        fdt_ptr = 0x40000000;
    }

    extern void riscv_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    if (fdt_is_valid((void*)fdt_ptr)) {
        riscv_fdt_parse_common(&boot, (const void*)fdt_ptr);
    } else {
        kernel_panic("FDT NOT FOUND AT FALLBACK!");
    }

    kernel_main_common(&boot);
}
