#include "boot/boot_info.h"
#include "kernel.h"
#include "hal/fdt_parser.h"
#include "hal/hal.h"
#include "hal/riscv_bsp.h"
#include <stdint.h>
#include <stddef.h>

extern void hal_serial_write(const char *s);
extern void hal_serial_init(void);

// Direct SBI ecall for putchar — avoids header conflicts with sbi.h's
// kernel_main declaration. Uses legacy SBI extension 0x01 (sbi_console_putchar).
static void riscv32_sbi_putchar(char c) {
    register long a0 __asm__("a0") = (long)c;
    register long a7 __asm__("a7") = 1; // SBI_EXT_0_1_CONSOLE_PUTCHAR
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

static void riscv32_early_puts(const char *s) {
    while (*s) {
        riscv32_sbi_putchar(*s++);
    }
}

// Early boot entry for RISC-V 32
// boot.S calls: kernel_main(hart_id=a0, fdt_ptr=a1)
// sbi.h declares kernel_main(uint64_t hartid, uint64_t device_tree_ptr)
// We match that exact signature here to avoid redeclaration conflicts.
void kernel_main(uint64_t hart_id, uint64_t fdt_ptr) {
    // Emit boot marker via SBI — works before any UART driver init
    riscv32_early_puts("BOOT: kernel_main reached\n");

    static boot_info_t boot;
    boot_info_init(&boot);
    boot.boot_cpu_id = hart_id;
    boot.arch = BOOT_ARCH_RISCV32;

    hal_riscv_set_boot_info(hart_id, fdt_ptr);

    // Now init UART driver for subsequent output
    hal_serial_init();

    if (fdt_ptr != 0 && fdt_is_valid((void*)(uintptr_t)fdt_ptr)) {
        extern void riscv_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
        riscv_fdt_parse_common(&boot, (const void*)(uintptr_t)fdt_ptr);
    }

    kernel_main_common(&boot);
}
