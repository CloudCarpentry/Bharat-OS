#include "hal/hal_timer.h"
#include "hal/hal_boot.h"

#define SBI_EXT_TIME 0x54494D45
#define SBI_EXT_TIME_SET_TIMER 0

struct sbiret {
    long error;
    long value;
};

static inline struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                                      unsigned long arg1, unsigned long arg2,
                                      unsigned long arg3, unsigned long arg4,
                                      unsigned long arg5) {
    struct sbiret ret;
    register unsigned long a0 asm("a0") = arg0;
    register unsigned long a1 asm("a1") = arg1;
    register unsigned long a2 asm("a2") = arg2;
    register unsigned long a3 asm("a3") = arg3;
    register unsigned long a4 asm("a4") = arg4;
    register unsigned long a5 asm("a5") = arg5;
    register unsigned long a6 asm("a6") = fid;
    register unsigned long a7 asm("a7") = ext;
    asm volatile("ecall"
                 : "+r"(a0), "+r"(a1)
                 : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                 : "memory");
    ret.error = a0;
    ret.value = a1;
    return ret;
}

static inline uint64_t read_time(void) {
    uint64_t time;
    __asm__ volatile("csrr %0, time" : "=r"(time));
    return time;
}

int hal_timer_init(uint32_t tick_hz) {
    // Rely on mtime CSR provided by the platform
    return 0;
}

int hal_timer_init_cpu_local(uint32_t tick_hz) {
    // Read current time, add frequency/hz and schedule
    // We assume an explicit set_periodic or set_oneshot will do it
    return 0;
}

int hal_timer_set_periodic(uint32_t tick_hz) {
    // In multikernel, we emulate periodic with recurring oneshots
    uint64_t next_tick = read_time() + (10000000 / tick_hz); // Assuming 10MHz timebase
    sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, next_tick, 0, 0, 0, 0, 0);
    return 0;
}

int hal_timer_set_oneshot(uint64_t ns_delay) {
    // Calculate ticks based on timebase
    uint64_t next_tick = read_time() + (ns_delay / 100);
    sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, next_tick, 0, 0, 0, 0, 0);
    return 0;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return read_time();
}
