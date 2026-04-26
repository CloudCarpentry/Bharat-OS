#ifndef BHARAT_UAPI_ARCH_RISCV32_SYSCALL_H
#define BHARAT_UAPI_ARCH_RISCV32_SYSCALL_H

#ifndef BHARAT_ARCH_RISCV32_SYSCALL_RUNTIME_READY
#warning "riscv32 syscall runtime is not enabled yet"
#endif

#include <stdint.h>

static inline long bharat_syscall_arch(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    register long a0 __asm__("a0") = arg1;
    register long a1 __asm__("a1") = arg2;
    register long a2 __asm__("a2") = arg3;
    register long a3 __asm__("a3") = arg4;
    register long a4 __asm__("a4") = arg5;
    register long a5 __asm__("a5") = arg6;
    register long a7 __asm__("a7") = sysno;

    __asm__ volatile(
        "ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
        : "memory"
    );

    return a0;
}

#endif /* BHARAT_UAPI_ARCH_RISCV32_SYSCALL_H */
