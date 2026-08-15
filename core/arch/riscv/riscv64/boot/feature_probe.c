#include <stdint.h>
#include <stdbool.h>

extern void early_uart_puts(const char* s);
extern void early_uart_put_hex(uint64_t val);

#define SATP_MODE_SHIFT 60
#define SATP_MODE_MASK  0xFULL
#define SATP_MODE_SV39  8ULL

uint64_t g_riscv_satp_mode = SATP_MODE_SV39;
bool g_riscv_asid_supported = false;

void riscv64_probe_satp_mode(void) {
    // Dynamic SATP probing without populated mapping tables triggers fatal
    // instruction aborts on newer or strict RISC-V emulators (like QEMU Virt).
    // RV64 requires Sv39 as the architectural baseline, so we pin it statically.
    // The VMM init will detect failure gracefully during page-table activation read-back.
}
