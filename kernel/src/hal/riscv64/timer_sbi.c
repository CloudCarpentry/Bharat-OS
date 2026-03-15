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

void hal_timer_init(void) {
    // Rely on mtime CSR provided by the platform
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable STIE (Supervisor Timer Interrupt Enable) in sie
    __asm__ volatile("csrs sie, %0" : : "r"(32)); // STIE is bit 5
}

void hal_timer_program_periodic(uint64_t ns) {
    // Emulate periodic with recurring oneshots
    // Assuming 10MHz timebase (typical for QEMU/RISC-V)
    uint64_t ticks = (10000000ULL * ns) / 1000000000ULL;
    uint64_t next_tick = read_time() + ticks;
    sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, next_tick, 0, 0, 0, 0, 0);
}

void hal_timer_program_oneshot(uint64_t ns) {
    // Calculate ticks based on timebase
    uint64_t ticks = (10000000ULL * ns) / 1000000000ULL;
    uint64_t next_tick = read_time() + ticks;
    sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, next_tick, 0, 0, 0, 0, 0);
}

uint64_t hal_timer_read_counter(void) {
    return read_time();
}

uint64_t hal_timer_read_freq(void) {
    return 10000000ULL; // Return 10MHz assuming generic RISC-V timebase
}

uint64_t hal_timer_monotonic_ticks(void) {
    return read_time();
}

bool hal_timer_is_per_cpu(void) {
    return true; // RISC-V timer (mtimecmp via SBI) is per-hart
}
