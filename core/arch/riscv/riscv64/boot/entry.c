#include "boot/boot_info.h"
#include "boot/adapters/opensbi_adapter.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/hal_boot.h"
#include "hal/riscv_bsp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*
 * Direct SBI ecall for putchar — bypasses all driver infrastructure so the
 * boot marker is visible on the console even if hal_serial_init() fails.
 * Uses legacy SBI extension 0x01 (sbi_console_putchar).
 */
static void riscv64_sbi_putchar(char c) {
    register long a0 __asm__("a0") = (long)c;
    register long a7 __asm__("a7") = 1; /* SBI_EXT_0_1_CONSOLE_PUTCHAR */
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

static void riscv64_early_puts(const char *s) {
    while (*s) {
        riscv64_sbi_putchar(*s++);
    }
}

// Early boot entry for RISC-V 64
// OpenSBI calls here in S-mode with: a0=hart_id, a1=fdt_phys_addr
void kernel_main(uint64_t hart_id, uintptr_t fdt_ptr) {
    /* Emit boot marker via SBI before any driver init so the smoke runner
     * can detect it even if UART driver setup fails. */
    riscv64_early_puts("BOOT: kernel_main reached\n");

    boot_info_t boot;
    if (opensbi_adapter_parse(hart_id, (const void *)fdt_ptr, &boot) != 0) {
        kernel_panic("OpenSBI/FDT handoff parse failed");
    }

    hal_serial_init();
    hal_serial_write("RISCV64 Boot Started\n");
    hal_serial_write("FDT ptr: ");
    hal_serial_write_hex(fdt_ptr);
    hal_serial_write("\n");

    if (fdt_ptr == 0 || !fdt_is_valid((void*)fdt_ptr)) {
        kernel_panic("FDT missing or invalid: boot contract violation");
    }

    hal_riscv_set_boot_info(hart_id, (uint64_t)fdt_ptr);

    extern void riscv_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
    riscv_fdt_parse_common(&boot, (const void*)fdt_ptr);
    boot.boot_cpu_id = hart_id;
    boot.arch = BOOT_ARCH_RISCV64;

    kernel_main_common(&boot);
}
