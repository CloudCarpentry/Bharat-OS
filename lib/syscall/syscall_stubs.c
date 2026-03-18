#include <bharat/syscalls.h>
#include <bharat/uapi/syscall_nr.h>

/* Generic dispatch layer resolving to per-arch implementations */
#if defined(__x86_64__)
#include <bharat/uapi/arch/x86_64/syscall.h>
#elif defined(__aarch64__)
#include <bharat/uapi/arch/arm64/syscall.h>
#elif defined(__riscv)
#include <bharat/uapi/arch/riscv64/syscall.h>
#else
static inline long bharat_syscall_arch(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    return -38; /* ENOSYS */
}
#endif

void bharat_exit(int code) {
    bharat_syscall_arch(SYSCALL_THREAD_DESTROY, code, 0, 0, 0, 0, 0);
    while (1) {}
}

int bharat_sched_thread_create(void (*entry)(void), uint64_t* out_tid) {
    return (int)bharat_syscall_arch(SYSCALL_THREAD_CREATE, (long)entry, (long)out_tid, 0, 0, 0, 0);
}

int bharat_sched_thread_destroy(uint64_t tid) {
    return (int)bharat_syscall_arch(SYSCALL_THREAD_DESTROY, (long)tid, 0, 0, 0, 0, 0);
}

int bharat_sched_yield(void) {
    return (int)bharat_syscall_arch(SYSCALL_SCHED_YIELD, 0, 0, 0, 0, 0, 0);
}

int bharat_sched_sleep(uint64_t millis) {
    return (int)bharat_syscall_arch(SYSCALL_SCHED_SLEEP, (long)millis, 0, 0, 0, 0, 0);
}

int bharat_sched_set_priority(uint64_t tid, uint32_t priority) {
    return (int)bharat_syscall_arch(SYSCALL_SCHED_SET_PRIORITY, (long)tid, (long)priority, 0, 0, 0, 0);
}

int bharat_sched_set_affinity(uint64_t tid, uint32_t affinity_mask) {
    return (int)bharat_syscall_arch(SYSCALL_SCHED_SET_AFFINITY, (long)tid, (long)affinity_mask, 0, 0, 0, 0);
}
