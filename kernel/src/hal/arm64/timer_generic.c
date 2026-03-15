#include "hal/hal_timer.h"
#include "hal/hal_boot.h"

static inline uint64_t read_cntpct(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (val));
    return val;
}

static inline void write_cntp_tval(uint32_t val) {
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (val));
}

static inline void write_cntp_ctl(uint32_t val) {
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (val));
}

void hal_timer_init(void) {
    // Generic timer is always on, frequency in CNTFRQ_EL0
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable timer, unmask interrupt
    write_cntp_ctl(1);
}

void hal_timer_program_periodic(uint64_t ns) {
    uint64_t frq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (frq));
    uint32_t ticks = (uint32_t)((frq * ns) / 1000000000ULL);
    write_cntp_tval(ticks);
}

void hal_timer_program_oneshot(uint64_t ns) {
    uint64_t frq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (frq));
    // Calculate ticks from ns delay
    uint32_t ticks = (uint32_t)((frq * ns) / 1000000000ULL);
    write_cntp_tval(ticks);
}

uint64_t hal_timer_read_counter(void) {
    return read_cntpct();
}

uint64_t hal_timer_read_freq(void) {
    uint64_t frq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (frq));
    return frq;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return read_cntpct();
}

bool hal_timer_is_per_cpu(void) {
    return true; // ARM Generic Timer is per-core
}
