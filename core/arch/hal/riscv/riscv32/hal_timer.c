#include "hal/hal_timer.h"
#include "hal/hal_ipi.h"
#include "../../arch/riscv/boot/sbi.h"

static uint32_t g_timer_timebase_freq_lo = 10000000UL;

// hal_timer_init: called once at boot to configure timer parameters
void hal_timer_init(void) {
    // QEMU virt RISC-V 32 has 10 MHz timebase (same as riscv64)
    g_timer_timebase_freq_lo = 10000000UL;
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable Supervisor Timer Interrupt (STIE) in sie CSR — bit 5
    __asm__ volatile("csrs sie, %0" : : "r"(32));
}

// hal_timer_isr: called by trap dispatcher on timer interrupt
void hal_timer_isr(void) {
    uint32_t current_time;
    __asm__ volatile("rdtime %0" : "=r"(current_time));
    // Reprogram for next tick (10ms at 10MHz = 100000 ticks)
    sbi_set_timer((uint64_t)current_time + (uint64_t)g_timer_timebase_freq_lo / 100UL);
    hal_timer_tick();
}

int hal_timer_source_init(uint32_t tick_hz) {
    if (tick_hz == 0U) return -1;
    uint32_t interval = g_timer_timebase_freq_lo / tick_hz;
    uint32_t current_time;
    __asm__ volatile("rdtime %0" : "=r"(current_time));
    sbi_set_timer((uint64_t)current_time + (uint64_t)interval);
    __asm__ volatile("csrs sie, %0" : : "r"(32));
    return 0;
}

void hal_timer_program_periodic(uint64_t ns) {
    uint32_t current_time;
    __asm__ volatile("rdtime %0" : "=r"(current_time));
    uint32_t ticks = (uint32_t)(((uint64_t)g_timer_timebase_freq_lo * ns) / 1000000000ULL);
    sbi_set_timer((uint64_t)current_time + (uint64_t)ticks);
}

void hal_timer_program_oneshot(uint64_t ns) {
    hal_timer_program_periodic(ns);
}

uint64_t hal_timer_read_counter(void) {
    uint32_t lo, hi;
    // Read time as two 32-bit halves (RISC-V32 has separate timeh CSR)
    do {
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
        __asm__ volatile("rdtime  %0" : "=r"(lo));
    } while (0);
    return ((uint64_t)hi << 32) | lo;
}

uint64_t hal_timer_read_freq(void) {
    return (uint64_t)g_timer_timebase_freq_lo;
}

uint64_t hal_timer_monotonic_ticks_arch(void) {
    return hal_timer_read_counter();
}

bool hal_timer_is_per_cpu(void) {
    return true;
}

// --- IPI (Software Interrupts via SBI) ---

void hal_ipi_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable Supervisor Software Interrupt (SSIE) in sie — bit 1
    __asm__ volatile("csrs sie, %0" : : "r"(2));
}

void hal_ipi_send(uint32_t target_cpu, hal_ipi_reason_t reason) {
    (void)reason;
    unsigned long hart_mask = (1UL << target_cpu);
    sbi_send_ipi(hart_mask, 0);
}

void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason) {
    (void)reason;
    unsigned long hart_mask = (unsigned long)mask;
    sbi_send_ipi(hart_mask, 0);
}
