#ifndef BHARAT_UAPI_ARCH_ARM32_SYSCALL_H
#define BHARAT_UAPI_ARCH_ARM32_SYSCALL_H

#ifndef BHARAT_ARCH_ARM32_SYSCALL_RUNTIME_READY
#warning "arm32 syscall runtime is not enabled yet"
#endif

#include <stdint.h>

static inline long bharat_syscall_arch(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    register long r0 __asm__("r0") = arg1;
    register long r1 __asm__("r1") = arg2;
    register long r2 __asm__("r2") = arg3;
    register long r3 __asm__("r3") = arg4;
    register long r4 __asm__("r4") = arg5;
    register long r5 __asm__("r5") = arg6;
    register long r7 __asm__("r7") = sysno;

    __asm__ volatile(
        "svc #0"
        : "+r"(r0)
        : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7)
        : "memory"
    );

    return r0;
}

#endif /* BHARAT_UAPI_ARCH_ARM32_SYSCALL_H */
