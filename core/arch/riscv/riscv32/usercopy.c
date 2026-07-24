#include "trap/usercopy.h"
#include "kernel/status.h"
#include <stddef.h>
#include <stdint.h>

#define BH_EX_UACCESS_RD 2
#define BH_EX_UACCESS_WR 3

kstatus_t arch_copy_from_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    uintptr_t prev_sstatus = 0;
    uintptr_t sum_bit = (1UL << 18);
    #if defined(__riscv)
    __asm__ __volatile__("csrr %0, sstatus" : "=r"(prev_sstatus) :: "memory");
    __asm__ __volatile__("csrs sstatus, %0" :: "r"(sum_bit) : "memory");
    #endif

    kstatus_t status = K_OK;
    size_t i = 0;
    for (; i < len; i++) {
        uint8_t val;
        __asm__ __volatile__(
            "1: lbu %0, 0(%2)\n"
            "2:\n"
            ".section .fixup,\"ax\"\n"
            "3: li %1, %3\n"
            "   j 2b\n"
            ".previous\n"
            ".section __ex_table,\"a\"\n"
            "   .balign 4\n"
            "   .long 1b - .\n"
            "   .long 3b - .\n"
            "   .short %4\n"
            "   .short 0\n"
            ".previous\n"
            : "=r"(val), "+r"(status)
            : "r"((uintptr_t)src + i), "i"(K_ERR_FAULT), "i"(BH_EX_UACCESS_RD)
            : "memory"
        );
        if (status != K_OK) {
            break;
        }
        *((uint8_t *)dst + i) = val;
    }

    #if defined(__riscv)
    if (!(prev_sstatus & sum_bit)) {
        __asm__ __volatile__("csrc sstatus, %0" :: "r"(sum_bit) : "memory");
    }
    #endif

    return status;
}

kstatus_t arch_copy_to_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    uintptr_t prev_sstatus = 0;
    uintptr_t sum_bit = (1UL << 18);
    #if defined(__riscv)
    __asm__ __volatile__("csrr %0, sstatus" : "=r"(prev_sstatus) :: "memory");
    __asm__ __volatile__("csrs sstatus, %0" :: "r"(sum_bit) : "memory");
    #endif

    kstatus_t status = K_OK;
    size_t i = 0;
    for (; i < len; i++) {
        uint8_t val = *((const uint8_t *)src + i);
        __asm__ __volatile__(
            "1: sb %1, 0(%2)\n"
            "2:\n"
            ".section .fixup,\"ax\"\n"
            "3: li %0, %3\n"
            "   j 2b\n"
            ".previous\n"
            ".section __ex_table,\"a\"\n"
            "   .balign 4\n"
            "   .long 1b - .\n"
            "   .long 3b - .\n"
            "   .short %4\n"
            "   .short 0\n"
            ".previous\n"
            : "+r"(status)
            : "r"(val), "r"((uintptr_t)dst + i), "i"(K_ERR_FAULT), "i"(BH_EX_UACCESS_WR)
            : "memory"
        );
        if (status != K_OK) {
            break;
        }
    }

    #if defined(__riscv)
    if (!(prev_sstatus & sum_bit)) {
        __asm__ __volatile__("csrc sstatus, %0" :: "r"(sum_bit) : "memory");
    }
    #endif

    return status;
}
