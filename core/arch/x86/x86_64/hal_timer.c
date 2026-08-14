#include "hal/hal_timer.h"
#include "hal/hal_boot.h"

#define LAPIC_TIMER_DIV_OFFSET 0x3E0
#define LAPIC_TIMER_INIT_CNT_OFFSET 0x380
#define LAPIC_TIMER_CURR_CNT_OFFSET 0x390
#define LAPIC_TIMER_LVT_OFFSET 0x320

extern uint32_t* g_lapic_base; // from apic.c

static uint64_t g_tsc_freq = 0;
static bool g_has_invariant_tsc = false;

static inline void lapic_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_lapic_base + offset) = value;
}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void x86_cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

void hal_timer_init(void) {
    uint32_t eax, ebx, ecx, edx;

    // Check for invariant TSC (CPUID 0x80000007 EDX bit 8)
    x86_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000007) {
        x86_cpuid(0x80000007, 0, &eax, &ebx, &ecx, &edx);
        g_has_invariant_tsc = (edx & (1u << 8)) != 0;
    }

    // Try to get TSC frequency from CPUID 0x15
    x86_cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x15) {
        x86_cpuid(0x15, 0, &eax, &ebx, &ecx, &edx);
        if (eax != 0 && ebx != 0 && ecx != 0) {
            g_tsc_freq = ((uint64_t)ecx * ebx) / eax;
        }
    }
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
    return g_tsc_freq;
}

bool hal_timer_is_per_cpu(void) {
    return true; // LAPIC timer is per CPU
}

uint64_t hal_timer_monotonic_ticks_arch(void) {
    return rdtsc();
}

void hal_timer_arch_get_caps(hal_timer_caps_t *caps) {
    caps->has_counter = true;
    caps->has_monotonic_ns = (g_tsc_freq > 0) && g_has_invariant_tsc;
    caps->has_precise_oneshot = false; // Degraded: LAPIC frequency is uncalibrated.
    caps->has_native_absolute_deadline = false;
    caps->is_per_cpu = true;
}
