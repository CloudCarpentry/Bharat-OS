#include "trap/usercopy.h"
#include "kernel/status.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BH_EX_UACCESS_RD 2
#define BH_EX_UACCESS_WR 3

static bool arm64_pan_supported(void) {
    uint64_t mmfr1;

    __asm__ __volatile__("mrs %0, id_aa64mmfr1_el1" : "=r"(mmfr1));
    return ((mmfr1 >> 20U) & 0xFU) != 0U;
}

kstatus_t arch_copy_from_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    uint64_t prev_pan = 0;
    bool has_pan = arm64_pan_supported();
    #if defined(__aarch64__)
    if (has_pan) {
        __asm__ __volatile__(
            ".arch_extension pan\n\t"
            "mrs %0, pan"
            : "=r"(prev_pan) :: "memory"
        );
        __asm__ __volatile__(
            ".arch_extension pan\n\t"
            "msr pan, #0"
            ::: "memory"
        );
    }
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
            : "r"((uintptr_t)src + i), "i"(K_ERR_FAULT), "i"(BH_EX_UACCESS_RD)
            : "memory"
        );
        if (status != K_OK) {
            break;
        }
        *((uint8_t *)dst + i) = val;
    }

    #if defined(__aarch64__)
    if (has_pan) {
        __asm__ __volatile__(
            ".arch_extension pan\n\t"
            "msr pan, %0"
            :: "r"(prev_pan) : "memory"
        );
    }
    #endif

    return status;
}

kstatus_t arch_copy_to_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    uint64_t prev_pan = 0;
    bool has_pan = arm64_pan_supported();
    #if defined(__aarch64__)
    if (has_pan) {
        __asm__ __volatile__(
            ".arch_extension pan\n\t"
            "mrs %0, pan"
            : "=r"(prev_pan) :: "memory"
        );
        __asm__ __volatile__(
            ".arch_extension pan\n\t"
            "msr pan, #0"
            ::: "memory"
        );
    }
    #endif

    kstatus_t status = K_OK;
    size_t i = 0;
    for (; i < len; i++) {
        uint8_t val = *((const uint8_t *)src + i);
        __asm__ __volatile__(
            "1: strb %w1, [%2]\n"
            "2:\n"
            ".section .fixup,\"ax\"\n"
            "3: mov %w0, %3\n"
            "   b 2b\n"
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

    #if defined(__aarch64__)
    if (has_pan) {
        __asm__ __volatile__(
            ".arch_extension pan\n\t"
            "msr pan, %0"
            :: "r"(prev_pan) : "memory"
        );
    }
    #endif

    return status;
}
