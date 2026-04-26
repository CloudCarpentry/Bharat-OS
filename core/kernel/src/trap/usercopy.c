#include "trap/usercopy.h"
#include <stdbool.h>
#include "kernel/status.h"

/**
 * Stage 1.5 Usercopy Hardening:
 * - NULL pointer rejection (when len > 0)
 * - Pointer + length overflow checks
 * - Max copy size enforcement
 * - User range validation
 * - TODO: Stage 2: validate against current process aspace/VMA/page permissions.
 */

#define MAX_USER_COPY_SIZE (1024 * 1024 * 64) // 64MB limit for single copy

extern int trap_user_range_valid(uintptr_t ptr, size_t len);

static void* internal_memcpy(void* dst, const void* src, size_t n) {
    char* d = (char*)dst;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

kstatus_t copy_from_user_checked(void *dst, uintptr_t src, size_t len) {
    if (len == 0) return K_OK;
    if (!dst || src == 0) return K_ERR_INVALID_ARG;
    if (len > MAX_USER_COPY_SIZE) return K_ERR_PARTIAL; // Use PARTIAL or IPC_MSG_TOO_LARGE

    // Overflow check
    if (src + len < src) return K_ERR_FAULT;

    // Range check
    if (!trap_user_range_valid(src, len)) return K_ERR_FAULT;

    // TODO: fault-recoverable copy where arch supports it
    internal_memcpy(dst, (const void *)src, len);
    return K_OK;
}

kstatus_t copy_to_user_checked(uintptr_t dst, const void *src, size_t len) {
    if (len == 0) return K_OK;
    if (dst == 0 || !src) return K_ERR_INVALID_ARG;
    if (len > MAX_USER_COPY_SIZE) return K_ERR_PARTIAL;

    // Overflow check
    if (dst + len < dst) return K_ERR_FAULT;

    // Range check
    if (!trap_user_range_valid(dst, len)) return K_ERR_FAULT;

    // TODO: fault-recoverable copy where arch supports it
    internal_memcpy((void *)dst, src, len);
    return K_OK;
}

kstatus_t copy_user_string_checked(char *dst, uintptr_t src, size_t max_len) {
    if (max_len == 0) return K_OK;
    if (!dst || src == 0) return K_ERR_INVALID_ARG;
    if (max_len > MAX_USER_COPY_SIZE) return K_ERR_PARTIAL;

    if (!trap_user_range_valid(src, 1)) return K_ERR_FAULT;

    size_t len = 0;
    const char *s = (const char *)src;
    while (len < max_len) {
        // Repeated range check for safety during byte-by-byte copy
        if (!trap_user_range_valid((uintptr_t)&s[len], 1)) return K_ERR_FAULT;

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
