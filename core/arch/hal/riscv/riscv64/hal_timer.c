#include "hal/hal_timer.h"
#include "hal/hal_ipi.h"
#include "../../arch/riscv/boot/sbi.h"

static uint64_t g_timer_timebase_freq = 10000000ULL;

void hal_timer_init(void) {
    // The fallback timebase frequency is suitable only for explicitly
    // known target profiles. Generic production capability must remain
    // degraded until platform/FDT discovery supplies the actual timebase.
    // Follow-up: Platform-Discovered RISC-V Timebase.
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
    uint64_t ticks;
    if (hal_timer_ns_to_ticks_ceil(ns, g_timer_timebase_freq, &ticks)) {
        sbi_set_timer(current_time + ticks);
    }
}

uint64_t hal_timer_read_counter(void) {
    uint64_t current_time;
    __asm__ volatile("rdtime %0" : "=r"(current_time));
    return current_time;
}

uint64_t hal_timer_read_freq(void) {
    return g_timer_timebase_freq;
}

uint64_t hal_timer_monotonic_ticks_arch(void) {
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
    sbi_send_ipi(hart_mask, 0); // SBI v0.2+ IPI
}

void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason) {
    (void)reason;
    unsigned long hart_mask = (unsigned long)mask;
    sbi_send_ipi(hart_mask, 0); // SBI v0.2+ IPI
}

void hal_timer_arch_get_caps(hal_timer_caps_t *caps) {
    caps->has_counter = true;
    caps->has_monotonic_ns = false; // Degraded: currently uses 10MHz generic guess. Follow-up: Platform-Discovered RISC-V Timebase.
    caps->has_precise_oneshot = false; // Degraded: untested precision due to missing calibration.
    caps->has_native_absolute_deadline = false;
    caps->is_per_cpu = true;
}
