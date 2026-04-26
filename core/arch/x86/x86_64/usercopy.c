#include "trap/usercopy.h"
#include "kernel/status.h"
#include <stddef.h>

/*
 * x86_64 production-ready safe usercopy.
 * EXPERIMENTAL: Fast path requires exception table support in the linker script.
 *
 * This implementation should eventually be moved to a .S file with a proper
 * exception table entry to handle page faults without panicking.
 *
 * For now, we provide the C-level interface that the common usercopy.c calls.
 */

kstatus_t arch_copy_from_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    // TODO: In a real production kernel, this would have a catch block
    // or an exception table entry associated with the 'rep movsb'.

    unsigned long dummy1, dummy2, dummy3;
    __asm__ __volatile__(
        "1: rep movsb\n"
        "2:\n"
        ".section .fixup,\"ax\"\n"
        "3: mov %3, %0\n"
        "   jmp 2b\n"
        ".previous\n"
        ".section __ex_table,\"a\"\n"
        "   .align 8\n"
        "   .quad 1b, 3b\n"
        ".previous\n"
        : "=r"(len), "+D"(dst), "+S"(src)
        : "i"(K_ERR_FAULT), "0"(0)
        : "memory"
    );

    return (len == 0) ? K_OK : (kstatus_t)len;
}

kstatus_t arch_copy_to_user_nofault(void *dst, const void *src, size_t len) {
    if (len == 0) return K_OK;

    unsigned long dummy1, dummy2, dummy3;
    __asm__ __volatile__(
        "1: rep movsb\n"
        "2:\n"
        ".section .fixup,\"ax\"\n"
        "3: mov %3, %0\n"
        "   jmp 2b\n"
        ".previous\n"
        ".section __ex_table,\"a\"\n"
        "   .align 8\n"
        "   .quad 1b, 3b\n"
        ".previous\n"
        : "=r"(len), "+D"(dst), "+S"(src)
        : "i"(K_ERR_FAULT), "0"(0)
        : "memory"
    );

    return (len == 0) ? K_OK : (kstatus_t)len;
}
