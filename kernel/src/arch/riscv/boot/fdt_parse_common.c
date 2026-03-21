#include "bharat/boot_info.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "kernel.h"

// FDT Parsing helpers for RISC-V families
void riscv_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr) {
    if (!boot || !fdt_ptr) return;

    if (fdt_is_valid(fdt_ptr)) {
        fdt_parse_discovery(fdt_ptr, hal_get_system_discovery());
        boot->booted_via_fdt = true;
    }
}
