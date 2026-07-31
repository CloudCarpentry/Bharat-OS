#include <bharat/uapi/syscall_nr.h>

#if defined(__x86_64__)
#include <bharat/uapi/arch/x86_64/syscall.h>
#elif defined(__aarch64__)
#include <bharat/uapi/arch/arm64/syscall.h>
#elif defined(__riscv)
#if __riscv_xlen == 64
#include <bharat/uapi/arch/riscv64/syscall.h>
#else
#include <bharat/uapi/arch/riscv32/syscall.h>
#endif
#elif defined(__arm__)
#include <bharat/uapi/arch/arm32/syscall.h>
#else
#error "Unsupported architecture"
#endif

long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    return bharat_syscall_arch(sysno, arg1, arg2, arg3, arg4, arg5, arg6);
}

#include <bharat/syscalls.h>
#include <bharat/uapi/syscall/bh_syscall_numbers.h>

int bharat_sched_yield(void) {
    return (int)bh_syscall(BH_SYS_SCHED_YIELD, 0, 0, 0, 0, 0, 0);
}
