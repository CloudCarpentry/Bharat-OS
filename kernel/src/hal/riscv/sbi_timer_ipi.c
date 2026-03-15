#include "hal/hal_timer.h"
#include "hal/hal_ipi.h"
#include "../../boot/riscv/sbi.h"

static uint64_t g_timer_timebase_freq = 10000000ULL;

void hal_timer_init(void) {
    // Assuming a timebase frequency of 10MHz (e.g. QEMU Virt and Shakti default)
    // Real implementation would parse device tree to get timebase-frequency
    g_timer_timebase_freq = 10000000ULL;
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable Supervisor Timer Interrupt (STIE) in sie CSR
    // M-mode would use mie
#ifdef CONFIG_RISCV_M_MODE
    __asm__ volatile("csrs mie, %0" : : "r"(32)); // MTIE is bit 5
#else
    __asm__ volatile("csrs sie, %0" : : "r"(32)); // STIE is bit 5
#endif
}

void hal_timer_program_periodic(uint64_t ns) {
    uint64_t current_time;
    __asm__ volatile("rdtime %0" : "=r"(current_time));
    uint64_t ticks = (g_timer_timebase_freq * ns) / 1000000000ULL;
    sbi_set_timer(current_time + ticks);
}

void hal_timer_program_oneshot(uint64_t ns) {
    uint64_t current_time;
    __asm__ volatile("rdtime %0" : "=r"(current_time));
    uint64_t ticks = (g_timer_timebase_freq * ns) / 1000000000ULL;
    sbi_set_timer(current_time + ticks);
}

uint64_t hal_timer_read_counter(void) {
    uint64_t current_time;
    __asm__ volatile("rdtime %0" : "=r"(current_time));
    return current_time;
}

uint64_t hal_timer_read_freq(void) {
    return g_timer_timebase_freq;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return hal_timer_read_counter();
}

bool hal_timer_is_per_cpu(void) {
    return true; // RISC-V local timer is per-hart
}

void hal_ipi_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable Software Interrupts
#ifdef CONFIG_RISCV_M_MODE
    __asm__ volatile("csrs mie, %0" : : "r"(8)); // MSIE is bit 3
#else
    __asm__ volatile("csrs sie, %0" : : "r"(2)); // SSIE is bit 1
#endif
}

void hal_ipi_send(uint32_t target_cpu, hal_ipi_reason_t reason) {
    // Note: SBI send_ipi takes a hart_mask.
    // reason encoding might need custom protocol in shared memory.
    // For now we map to standard IPI since reasons are typically processed softly.
    (void)reason;
    unsigned long hart_mask = (1UL << target_cpu);
    sbi_send_ipi_payload(&hart_mask, 0); // Payload is ignored by basic SBI IPI
}

void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason) {
    (void)reason;
    unsigned long hart_mask = (unsigned long)mask;
    sbi_send_ipi_payload(&hart_mask, 0);
}
