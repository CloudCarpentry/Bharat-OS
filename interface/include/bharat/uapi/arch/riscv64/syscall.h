#ifndef BHARAT_UAPI_ARCH_RISCV64_SYSCALL_H
#define BHARAT_UAPI_ARCH_RISCV64_SYSCALL_H

#include <bharat/uapi/syscall_nr.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RISC-V 64 Syscall Convention:
 * System call number in a7
 * Arguments in a0-a5
 * Return value in a0
 */
static inline long bharat_syscall_arch(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long a7 __asm__("a7") = n;
    register long a0 __asm__("a0") = a1;
    register long a1_r __asm__("a1") = a2;
    register long a2_r __asm__("a2") = a3;
    register long a3_r __asm__("a3") = a4;
    register long a4_r __asm__("a4") = a5;
    register long a5_r __asm__("a5") = a6;

    __asm__ volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a7), "r" (a1_r), "r" (a2_r), "r" (a3_r), "r" (a4_r), "r" (a5_r)
        : "memory"
    );
    return a0;
}

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_ARCH_RISCV64_SYSCALL_H */
