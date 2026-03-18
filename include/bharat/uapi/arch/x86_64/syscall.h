#ifndef BHARAT_UAPI_ARCH_X86_64_SYSCALL_H
#define BHARAT_UAPI_ARCH_X86_64_SYSCALL_H

#include <bharat/uapi/syscall_nr.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * x86_64 Syscall Convention:
 * System call number in %rax
 * Arguments in %rdi, %rsi, %rdx, %r10, %r8, %r9
 * Return value in %rax
 * %rcx and %r11 are clobbered by the syscall instruction
 */
static inline long bharat_syscall_arch(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    register long r9 __asm__("r9") = a6;
    __asm__ volatile (
        "syscall"
        : "=a" (ret)
        : "a" (n), "D" (a1), "S" (a2), "d" (a3), "r" (r10), "r" (r8), "r" (r9)
        : "rcx", "r11", "memory"
    );
    return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_ARCH_X86_64_SYSCALL_H */
