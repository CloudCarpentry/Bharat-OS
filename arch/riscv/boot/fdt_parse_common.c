#include "boot/boot_info.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "kernel.h"

// FDT Parsing helpers for RISC-V families
void riscv_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr) {
    if (!boot || !fdt_ptr) return;

    if (fdt_is_valid(fdt_ptr)) {
        system_discovery_t* discovery = hal_get_system_discovery();
        fdt_parse_discovery(fdt_ptr, discovery);
        boot->source == BOOT_SOURCE_FDT = true;

        if (discovery->boot_video.valid) {
            boot->video = discovery->boot_video;
        }
    }
}
