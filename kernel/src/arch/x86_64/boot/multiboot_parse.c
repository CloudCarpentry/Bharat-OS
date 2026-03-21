#include <stdint.h>
#include <stddef.h>
#include "bharat/boot_info.h"
#include "boot/x86_64/multiboot2.h"
#include "boot/platform_boot_info.h"

// Forward declaration from arch code
extern void x86_64_parse_multiboot_framebuffer(multiboot_information_t *mb_info);

void x86_64_parse_multiboot(uint32_t magic, multiboot_information_t *mb_info, platform_boot_info_t *plat_info, boot_info_t *boot_info) {
    if (!mb_info || !boot_info || !plat_info) return;

    // This is a minimal parser that safely ignores unknown tags rather than making
    // permissive assumptions. A full implementation would map actual hardware details.

    // For now, conservatively invalidate the video to force serial/text console
    // because full robust validation of multiboot video tags is missing here.
    boot_info->video.valid = false;
    plat_info->has_early_console = true; // Fallback to safe serial defaults
}
