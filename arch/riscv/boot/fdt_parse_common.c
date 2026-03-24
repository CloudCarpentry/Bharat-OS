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
        boot->source = BOOT_SOURCE_OPENSBI_FDT;

        if (discovery->boot_video.valid) {
            boot->console.type = BOOT_CONSOLE_FRAMEBUFFER;
            boot->console.fb_phys_base = discovery->boot_video.phys_addr;
            boot->console.fb_width = discovery->boot_video.width;
            boot->console.fb_height = discovery->boot_video.height;
            boot->console.fb_pitch = discovery->boot_video.stride_bytes;
            boot->console.fb_bpp = 32;
        } else {
            boot->console.type = BOOT_CONSOLE_NONE;
        }
    }
}
