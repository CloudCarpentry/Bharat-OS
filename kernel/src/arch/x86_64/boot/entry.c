#include "bharat/boot_info.h"
#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "boot/x86_64/multiboot2.h"
#include "boot/platform_boot_info.h"

// Forward declaration for x86_64 multiboot parsing
extern void x86_64_parse_multiboot(uint32_t magic, multiboot_information_t *mb_info, platform_boot_info_t *plat_info, boot_info_t *boot_info);

// Architecture-specific entry point called from boot.S
void kernel_main(uint32_t magic, multiboot_information_t *mb_info) {
    boot_info_t boot = {0};
    platform_boot_info_t plat = {0};

    boot.magic = magic;

    // Check multiboot magic
    if (magic != 0x36d76289) { // MULTIBOOT2_BOOTLOADER_MAGIC
        kernel_panic("Invalid Multiboot2 magic number!");
    }

    // Parse multiboot into platform and generic boot info
    x86_64_parse_multiboot(magic, mb_info, &plat, &boot);

    // Provide generic hints
    boot.booted_via_multiboot = true;

    // Call the common kernel main
    kernel_main_common(&boot);
}
