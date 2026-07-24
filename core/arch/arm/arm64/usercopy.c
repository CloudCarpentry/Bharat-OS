#include "trap/usercopy.h"
#include "kernel/status.h"
#include <stddef.h>
#include <stdint.h>

#define BH_EX_UACCESS_RD 2
#define BH_EX_UACCESS_WR 3

kstatus_t arch_copy_from_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    #if defined(__aarch64__)
    __asm__ __volatile__("msr pan, #0" ::: "memory");
    #endif

    kstatus_t status = K_OK;
    size_t i = 0;
    for (; i < len; i++) {
        uint8_t val;
        __asm__ __volatile__(
            "1: ldrb %w0, [%2]\n"
            "2:\n"
            ".section .fixup,\"ax\"\n"
            "3: mov %w1, %3\n"
            "   b 2b\n"
            ".previous\n"
            ".section __ex_table,\"a\"\n"
            "   .balign 4\n"
            "   .long 1b - .\n"
            "   .long 3b - .\n"
            "   .short %4\n"
            "   .short 0\n"
            ".previous\n"
            : "=r"(val), "+r"(status)
            : "r"((uintptr_t)src + i), "i"(K_ERR_FAULT), "i"(BH_EX_UACCESS_RD), "1"(status)
            : "memory"
        );
        if (status != K_OK) {
            break;
        }
        *((uint8_t *)dst + i) = val;
    }

    #if defined(__aarch64__)
    __asm__ __volatile__("msr pan, #1" ::: "memory");
    #endif

    return status;
}

kstatus_t arch_copy_to_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    #if defined(__aarch64__)
    __asm__ __volatile__("msr pan, #0" ::: "memory");
    #endif

    kstatus_t status = K_OK;
    size_t i = 0;
    for (; i < len; i++) {
        uint8_t val = *((const uint8_t *)src + i);
        __asm__ __volatile__(
            "1: strb %w2, [%1]\n"
            "2:\n"
            ".section .fixup,\"ax\"\n"
            "3: mov %w0, %4\n"
            "   b 2b\n"
            ".previous\n"
            ".section __ex_table,\"a\"\n"
            "   .balign 4\n"
            "   .long 1b - .\n"
            "   .long 3b - .\n"
            "   .short %5\n"
            "   .short 0\n"
            ".previous\n"
            : "+r"(status)
            : "r"((uintptr_t)dst + i), "r"(val), "i"(K_ERR_FAULT), "i"(BH_EX_UACCESS_WR), "r"(status)
            : "memory"
        );
        if (status != K_OK) {
            break;
        }
    }

    #if defined(__aarch64__)
    __asm__ __volatile__("msr pan, #1" ::: "memory");
    #endif

    return status;
}
