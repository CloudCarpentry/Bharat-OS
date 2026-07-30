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

    if (fdt_ptr == 0) fdt_ptr = 0x40000000;

    static boot_info_t boot;
    boot_info_init(&boot);
    hal_serial_write("FDT Ptr: ");
    hal_serial_write_hex(fdt_ptr);
    hal_serial_write("\n");

    extern void arm_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    arm_fdt_parse_common(&boot, (const void*)fdt_ptr);

    boot.source = BOOT_SOURCE_LEGACY_LOADER;
    boot.arch = BOOT_ARCH_ARM64;

    kernel_main_common(&boot);
}
