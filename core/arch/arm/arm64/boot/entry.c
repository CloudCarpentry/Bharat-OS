#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/hal_boot.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "drivers/serial/uart_driver.h"
#include "debug/early_console.h"

// Early boot entry for ARM64
void kernel_main(uintptr_t fdt_ptr) {
    /* boot.S has already enabled the PL011.  Keep this path allocation-free
     * and use the raw UART until kernel_main_common() discovers and registers
     * the normal console backend.  Binding the profile-based UART here used
     * indirect driver calls before the early console was initialized and
     * could trap immediately after this marker on QEMU's ARM virt machine. */
    hal_serial_write("BOOT: kernel_main reached\n");

    if (!fdt_is_valid((const void *)fdt_ptr)) {
        for (uintptr_t addr = 0x40000000; addr < 0x80000000; addr += 0x10000) {
            if (fdt_is_valid((const void *)addr)) {
                fdt_ptr = addr;
                break;
            }
        }
    }

    static boot_info_t boot;
    boot_info_init(&boot);
    hal_serial_write("FDT Ptr: ");
    hal_serial_write_hex(fdt_ptr);
    hal_serial_write("\n");

    extern void arm_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    arm_fdt_parse_common(&boot, (const void*)fdt_ptr);

    if (boot.module_count == 0) {
        /*
         * Fallback for direct simulator / bare-metal boot:
         * Check if an init module container (magic 0xB4A2D1A5) is staged
         * at standard RAM offset 0x48000000.
         */
        const volatile uint32_t *staged = (const volatile uint32_t *)0x48000000ULL;
        if (*staged == 0xB4A2D1A5) {
            uint64_t total_size = 0;
            const uint8_t *curr = (const uint8_t *)0x48000000ULL;
            while (total_size < 32 * 1024 * 1024) {
                const uint32_t *hdr = (const uint32_t *)curr;
                if (hdr[0] != 0xB4A2D1A5) break; // magic
                uint32_t payload_size = hdr[5]; // offset 20 (5 * 4) = payload_size
                if (payload_size == 0) break;
                uint64_t mod_len = 128ULL + payload_size;
                total_size += mod_len;
                curr += mod_len;
            }
            if (total_size > 0) {
                boot_info_add_module(&boot, 0x48000000ULL, total_size, "initrd");
            }
        }
    }

    boot.source = BOOT_SOURCE_LEGACY_LOADER;
    boot.arch = BOOT_ARCH_ARM64;

    kernel_main_common(&boot);
}
