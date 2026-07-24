#include "trap/usercopy.h"
#include "kernel/status.h"
#include <stddef.h>

#define BH_EX_UACCESS_RD 2
#define BH_EX_UACCESS_WR 3

/*
 * x86_64 production-ready safe usercopy.
 * Fast path uses exception table support in the linker script.
 */

kstatus_t arch_copy_from_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    __asm__ __volatile__(
        "1: rep movsb\n"
        "2:\n"
        ".section .fixup,\"ax\"\n"
        "3: mov %3, %0\n"
        "   jmp 2b\n"
        ".previous\n"
        ".section __ex_table,\"a\"\n"
        "   .align 4\n"
        "   .long 1b - .\n"
        "   .long 3b - .\n"
        "   .short %c4\n"
        "   .short 0\n"
        ".previous\n"
        : "+c"(len), "+D"(dst), "+S"(src)
        : "i"(K_ERR_FAULT), "i"(BH_EX_UACCESS_RD)
        : "memory"
    );

    return (len == 0) ? K_OK : (kstatus_t)len;
}

kstatus_t arch_copy_to_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    __asm__ __volatile__(
        "1: rep movsb\n"
        "2:\n"
        ".section .fixup,\"ax\"\n"
        "3: mov %3, %0\n"
        "   jmp 2b\n"
        ".previous\n"
        ".section __ex_table,\"a\"\n"
        "   .align 4\n"
        "   .long 1b - .\n"
        "   .long 3b - .\n"
        "   .short %c4\n"
        "   .short 0\n"
        ".previous\n"
        : "+c"(len), "+D"(dst), "+S"(src)
        : "i"(K_ERR_FAULT), "i"(BH_EX_UACCESS_WR)
        : "memory"
    );

    return (len == 0) ? K_OK : (kstatus_t)len;
}
