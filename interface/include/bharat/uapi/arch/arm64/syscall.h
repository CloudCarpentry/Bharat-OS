#ifndef BHARAT_UAPI_ARCH_ARM64_SYSCALL_H
#define BHARAT_UAPI_ARCH_ARM64_SYSCALL_H

#include <bharat/uapi/syscall_nr.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ARM64 Syscall Convention:
 * System call number in x8
 * Arguments in x0-x5
 * Return value in x0
 */
static inline long bharat_syscall_arch(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    register long x2 __asm__("x2") = a3;
    register long x3 __asm__("x3") = a4;
    register long x4 __asm__("x4") = a5;
    register long x5 __asm__("x5") = a6;

    __asm__ volatile (
        "svc #0"
        : "+r" (x0)
        : "r" (x8), "r" (x1), "r" (x2), "r" (x3), "r" (x4), "r" (x5)
        : "memory"
    );
    return x0;
}

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_ARCH_ARM64_SYSCALL_H */
