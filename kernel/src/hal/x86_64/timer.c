#include "hal/hal_timer.h"
#include "hal/hal_boot.h"

#define LAPIC_TIMER_DIV_OFFSET 0x3E0
#define LAPIC_TIMER_INIT_CNT_OFFSET 0x380
#define LAPIC_TIMER_CURR_CNT_OFFSET 0x390
#define LAPIC_TIMER_LVT_OFFSET 0x320

extern uint32_t* g_lapic_base; // from apic.c

static inline void lapic_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_lapic_base + offset) = value;
}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

int hal_timer_init(uint32_t tick_hz) {
    // Nothing needed for TSC source
    return 0;
}

int hal_timer_init_cpu_local(uint32_t tick_hz) {
    // Map LAPIC timer to vector 32, divide by 16
    lapic_write(LAPIC_TIMER_DIV_OFFSET, 0x03);
    lapic_write(LAPIC_TIMER_LVT_OFFSET, 32);
    return 0;
}

int hal_timer_set_periodic(uint32_t tick_hz) {
    lapic_write(LAPIC_TIMER_LVT_OFFSET, 32 | 0x20000); // Periodic mode
    lapic_write(LAPIC_TIMER_INIT_CNT_OFFSET, 1000000); // Example ticks
    return 0;
}

int hal_timer_set_oneshot(uint64_t ns_delay) {
    lapic_write(LAPIC_TIMER_LVT_OFFSET, 32); // One-shot mode
    lapic_write(LAPIC_TIMER_INIT_CNT_OFFSET, ns_delay / 100); // Example conv
    return 0;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return rdtsc();
}
