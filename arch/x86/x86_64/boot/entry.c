#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "multiboot2.h"
#include "boot/platform_boot_info.h"

#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "multiboot2.h"
#include "boot/platform_boot_info.h"

// Forward declaration for x86_64 multiboot parsing
extern void x86_64_parse_multiboot(uint32_t magic, multiboot_information_t *mb_info, platform_boot_info_t *plat_info, boot_info_t *boot_info);

extern void hal_serial_init(void);
extern void hal_serial_write(const char *s);

// Architecture-specific entry point called from boot.S
void kernel_main(uint32_t magic, multiboot_information_t *mb_info) {
    hal_serial_init();
    hal_serial_write("BOOT: kernel_main reached\n");

    boot_info_t boot;
    boot_info_init(&boot);

    platform_boot_info_t plat = {0};

    // Accept both Multiboot2 (QEMU/GRUB2) and Multiboot1 (legacy loaders).
    if (magic != 0x36d76289U && magic != 0x2badb002U) {
        hal_serial_write("BAD MAGIC: ");
        for (int i = 7; i >= 0; i--) {
            uint32_t nibble = (magic >> (i * 4)) & 0xF;
            char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
            char buf[2] = {c, '\0'};
            hal_serial_write(buf);
        }
        hal_serial_write("\n");
        kernel_panic("Invalid Multiboot magic number!");
    }

    // Parse multiboot into platform and generic boot info
    x86_64_parse_multiboot(magic, mb_info, &plat, &boot);

    // Provide generic hints
    boot.source = BOOT_SOURCE_MULTIBOOT2;
    // Removed arch_ptr as it is no longer in the canonical contract directly, adapters handle it.

    // Call the common kernel main
    kernel_main_common(&boot);
}
