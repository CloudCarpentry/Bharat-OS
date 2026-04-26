#include "trap/usercopy.h"
#include <stdbool.h>
#include "kernel/status.h"
#include "mm/user_range.h"

/**
 * Stage 1.5 Usercopy Hardening:
 * - NULL pointer rejection (when len > 0)
 * - Pointer + length overflow checks
 * - Max copy size enforcement
 * - User range validation
 * - Stage 2 (Adapter ready): validate against current process aspace/VMA/page permissions.
 */

#ifndef BH_USERCOPY_MAX_BYTES
#define BH_USERCOPY_MAX_BYTES 4096U
#endif

static void* internal_memcpy(void* dst, const void* src, size_t n) {
    char* d = (char*)dst;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

kstatus_t copy_from_user_checked(void *dst, uintptr_t src, size_t len) {
    if (len == 0) return K_OK;
    if (!dst || src == 0) return K_ERR_INVALID_ARG;
    if (len > BH_USERCOPY_MAX_BYTES) return K_ERR_OVERFLOW;

    // Overflow check before range validation
    if (src > UINTPTR_MAX - len) return K_ERR_FAULT;

    // Range check (Stage 2 adapter)
    kstatus_t st = mm_user_range_validate_current(src, len, BH_USER_ACCESS_READ);
    if (st != K_OK) return st;

    // TODO: fault-recoverable copy where arch supports it
    internal_memcpy(dst, (const void *)src, len);
    return K_OK;
}

kstatus_t copy_to_user_checked(uintptr_t dst, const void *src, size_t len) {
    if (len == 0) return K_OK;
    if (dst == 0 || !src) return K_ERR_INVALID_ARG;
    if (len > BH_USERCOPY_MAX_BYTES) return K_ERR_OVERFLOW;

    // Overflow check before range validation
    if (dst > UINTPTR_MAX - len) return K_ERR_FAULT;

    // Range check (Stage 2 adapter)
    kstatus_t st = mm_user_range_validate_current(dst, len, BH_USER_ACCESS_WRITE);
    if (st != K_OK) return st;

    // TODO: fault-recoverable copy where arch supports it
    internal_memcpy((void *)dst, src, len);
    return K_OK;
}

kstatus_t copy_user_string_checked(char *dst, uintptr_t src, size_t max_len) {
    if (max_len == 0) return K_OK;
    if (!dst || src == 0) return K_ERR_INVALID_ARG;
    if (max_len > BH_USERCOPY_MAX_BYTES) return K_ERR_OVERFLOW;

    kstatus_t st = mm_user_range_validate_current(src, 1, BH_USER_ACCESS_READ);
    if (st != K_OK) return st;

    size_t len = 0;
    const char *s = (const char *)src;
    while (len < max_len) {
        // Repeated range check for safety during byte-by-byte copy
        st = mm_user_range_validate_current((uintptr_t)&s[len], 1, BH_USER_ACCESS_READ);
        if (st != K_OK) return st;

        dst[len] = s[len];
        if (dst[len] == '\0') return K_OK;
        len++;
    }

    if (len == max_len) {
        dst[max_len - 1] = '\0';
        return K_ERR_OVERFLOW;
    }

    return K_OK;
}
