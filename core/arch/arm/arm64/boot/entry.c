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
    *(volatile uint32_t*)0x09000000 = 'K';

    if (fdt_ptr == 0) fdt_ptr = 0x40000000;

    static boot_info_t boot;
    boot_info_init(&boot);
    
    extern const uart_driver_ops_t uart_pl011_ops;
    static uart_device_t pl011_dev;
    pl011_dev.base = 0x09000000;
    pl011_dev.ops = &uart_pl011_ops;
    early_console_bind(&pl011_dev);

    hal_serial_write("BOOT: kernel_main reached\n");
    hal_serial_write("FDT Ptr: ");
    hal_serial_write_hex(fdt_ptr);
    hal_serial_write("\n");

    extern void arm_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    arm_fdt_parse_common(&boot, (const void*)fdt_ptr);

    boot.source = BOOT_SOURCE_LEGACY_LOADER;
    boot.arch = BOOT_ARCH_ARM64;

    kernel_main_common(&boot);
}
