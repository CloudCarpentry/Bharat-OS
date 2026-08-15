#include "hal/hal_timer.h"
#include "hal/hal_discovery.h"

static inline uint64_t arm32_read_cntpct(void) {
  uint32_t lo, hi;
  __asm__ volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
  return ((uint64_t)hi << 32) | lo;
}

static inline uint32_t arm32_read_cntfrq(void) {
  uint32_t frq;
  __asm__ volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(frq));
  return frq;
}

static inline void arm32_write_cntp_tval(uint32_t val) {
  __asm__ volatile("mcr p15, 0, %0, c14, c2, 0" : : "r"(val));
}

static inline void arm32_write_cntp_ctl(uint32_t val) {
  __asm__ volatile("mcr p15, 0, %0, c14, c2, 1" : : "r"(val));
}

static uint64_t g_arm32_timer_freq = 0;

void hal_timer_init(void) {
  uint32_t frq = arm32_read_cntfrq();
  if (frq > 0) {
    g_arm32_timer_freq = frq;
  } else {
    system_discovery_t *discovery = hal_get_system_discovery();
    if (discovery && discovery->timers[0].frequency > 0) {
      g_arm32_timer_freq = discovery->timers[0].frequency;
    } else {
      g_arm32_timer_freq = 62500000ULL; /* 62.5 MHz standard default */
    }
  }
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
  (void)cpu_id;
  arm32_write_cntp_ctl(1);
}

void hal_timer_program_periodic(uint64_t ns) {
  uint64_t frq = hal_timer_read_freq();
  uint32_t ticks = (uint32_t)((frq * ns) / 1000000000ULL);
  arm32_write_cntp_tval(ticks);
}

void hal_timer_program_oneshot(uint64_t ns) {
  uint64_t frq = hal_timer_read_freq();
  uint64_t ticks;
  if (hal_timer_ns_to_ticks_ceil(ns, frq, &ticks)) {
    if (ticks > UINT32_MAX) {
      ticks = UINT32_MAX;
    }
    arm32_write_cntp_tval((uint32_t)ticks);
  }
}

uint64_t hal_timer_read_counter(void) { return arm32_read_cntpct(); }

uint64_t hal_timer_read_freq(void) {
  if (g_arm32_timer_freq == 0) {
    uint32_t frq = arm32_read_cntfrq();
    if (frq > 0)
      return (uint64_t)frq;
    return 62500000ULL;
  }
  return g_arm32_timer_freq;
}

uint64_t hal_timer_monotonic_ticks_arch(void) { return arm32_read_cntpct(); }

bool hal_timer_is_per_cpu(void) { return true; }

void hal_timer_arch_get_caps(hal_timer_caps_t *caps) {
  caps->has_counter = true;
  caps->has_monotonic_ns = true;
  caps->has_precise_oneshot = true;
  caps->has_native_absolute_deadline = false;
  caps->is_per_cpu = true;
}
