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

void hal_timer_init(void) {
    // Determine frequency, etc.
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Map LAPIC timer to vector 32, divide by 16
    lapic_write(LAPIC_TIMER_DIV_OFFSET, 0x03);
    lapic_write(LAPIC_TIMER_LVT_OFFSET, 32);
}

void hal_timer_program_periodic(uint64_t ns) {
    uint32_t ticks = ns / 1000; // rough approximation for timer freq
    lapic_write(LAPIC_TIMER_LVT_OFFSET, 32 | 0x20000); // Periodic mode
    lapic_write(LAPIC_TIMER_INIT_CNT_OFFSET, ticks);
}

void hal_timer_program_oneshot(uint64_t ns) {
    uint32_t ticks = ns / 1000; // rough approximation for timer freq
    lapic_write(LAPIC_TIMER_LVT_OFFSET, 32); // One-shot mode
    lapic_write(LAPIC_TIMER_INIT_CNT_OFFSET, ticks);
}

uint64_t hal_timer_read_counter(void) {
    return rdtsc();
}

uint64_t hal_timer_read_freq(void) {
    return 1000000000ULL; // Return ~1GHz assuming generic TSC
}

uint64_t hal_timer_monotonic_ticks(void) {
    return rdtsc();
}

bool hal_timer_is_per_cpu(void) {
    return true; // LAPIC timer is per CPU
}
