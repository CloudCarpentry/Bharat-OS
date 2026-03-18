#include <stdint.h>
#include <stdbool.h>

extern void early_uart_puts(const char* s);
extern void early_uart_put_hex(uint64_t val);

#define SATP_MODE_SHIFT 60
#define SATP_MODE_MASK  0xFULL
#define SATP_MODE_SV39  8ULL

uint64_t g_riscv_satp_mode = 0;
bool g_riscv_asid_supported = false;

static inline void csr_write_satp(uintptr_t val) {
    asm volatile("csrw satp, %0" :: "r"(val) : "memory");
}

static inline uintptr_t csr_read_satp(void) {
    uintptr_t val;
    asm volatile("csrr %0, satp" : "=r"(val) : "memory");
    return val;
}

void riscv64_probe_satp_mode(void) {
    // Attempt to write Sv39 mode with ASID=0, PPN=0
    // PPN=0 is safe — we're not activating translation, just testing mode acceptance
    uintptr_t probe = SATP_MODE_SV39 << SATP_MODE_SHIFT;

    csr_write_satp(probe);
    uintptr_t readback = csr_read_satp();

    // Immediately disable — restore Bare mode
    csr_write_satp(0UL);

    uint64_t mode = (readback >> SATP_MODE_SHIFT) & SATP_MODE_MASK;

    if (mode != SATP_MODE_SV39) {
        // Nothing is initialized yet — use your lowest-level uart/serial output
        // early_uart_puts("[FATAL] SATP Sv39 not supported on this core. Detected mode: ");
        // early_uart_put_hex(mode);
        // early_uart_puts(". Bharat-OS requires Sv39 or higher.\n");
        for (;;) { asm volatile("wfi"); }  // halt, don't reset-loop
    }

    // Record for later use
    g_riscv_satp_mode = SATP_MODE_SV39;

    // Probe ASID support separately
    // Write a non-zero ASID (e.g. 1) to ASID field (bits 44-59 in Sv39)
    uintptr_t asid_probe = (1ULL << 44);
    csr_write_satp(asid_probe);
    uintptr_t asid_readback = csr_read_satp();
    csr_write_satp(0UL);

    if ((asid_readback >> 44) & 0xFFFFULL) {
        g_riscv_asid_supported = true;
    } else {
        g_riscv_asid_supported = false; // ASID silently discarded
    }
}
