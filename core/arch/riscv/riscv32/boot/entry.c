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
// OpenSBI jumps here with: a0 = hart_id, a1 = dtb_phys_addr (both 32-bit).
// Using uint32_t matches the RV32 register width and avoids the two-register
// pair ABI that uint64_t would require on RV32 (which would misread a1 as
// the high word of hart_id rather than fdt_ptr).
void kernel_main(uint32_t hart_id, uint32_t fdt_ptr) {
    // Emit boot marker via SBI — works before any UART driver init
    riscv32_early_puts("BOOT: kernel_main reached\n");

    static boot_info_t boot;
    boot_info_init(&boot);
    boot.boot_cpu_id = hart_id;
    boot.arch = BOOT_ARCH_RISCV32;

    hal_riscv_set_boot_info(hart_id, (uint64_t)fdt_ptr);

    // Now init UART driver for subsequent output
    hal_serial_init();

    if (fdt_ptr != 0 && fdt_is_valid((void*)fdt_ptr)) {
        extern void riscv_fdt_parse_common(boot_info_t *boot, const void *fdt_ptr);
        riscv_fdt_parse_common(&boot, (const void*)fdt_ptr);
    }

    kernel_main_common(&boot);
}
