#ifndef BHARAT_UAPI_ARCH_RISCV64_SYSCALL_H
#define BHARAT_UAPI_ARCH_RISCV64_SYSCALL_H

static inline long bharat_syscall_arch(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long a7 __asm__("a7") = n;
    register long a0 __asm__("a0") = a1;
    register long a1_reg __asm__("a1") = a2;
    register long a2_reg __asm__("a2") = a3;
    register long a3_reg __asm__("a3") = a4;
    register long a4_reg __asm__("a4") = a5;
    register long a5_reg __asm__("a5") = a6;
    __asm__ volatile (
        "ecall"
        : "=r"(a0)
        : "r"(a7), "r"(a0), "r"(a1_reg), "r"(a2_reg), "r"(a3_reg), "r"(a4_reg), "r"(a5_reg)
        : "memory"
    );
    return a0;
}

#endif /* BHARAT_UAPI_ARCH_RISCV64_SYSCALL_H */
