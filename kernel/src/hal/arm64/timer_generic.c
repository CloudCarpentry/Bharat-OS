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

int hal_timer_init(uint32_t tick_hz) {
    // Generic timer is always on, frequency in CNTFRQ_EL0
    return 0;
}

int hal_timer_init_cpu_local(uint32_t tick_hz) {
    // Enable timer, unmask interrupt
    write_cntp_ctl(1);
    return 0;
}

int hal_timer_set_periodic(uint32_t tick_hz) {
    // We prefer one-shot, but for periodic we'd read frq and div
    uint64_t frq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (frq));
    write_cntp_tval(frq / tick_hz);
    return 0;
}

int hal_timer_set_oneshot(uint64_t ns_delay) {
    uint64_t frq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (frq));
    // Calculate ticks from ns delay
    uint32_t ticks = (frq / 1000000000) * ns_delay;
    write_cntp_tval(ticks);
    return 0;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return read_cntpct();
}
