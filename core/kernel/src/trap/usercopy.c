#include "trap/usercopy.h"
#include <stdbool.h>

extern int trap_user_range_valid(uintptr_t ptr, size_t len);

static void* internal_memcpy(void* dst, const void* src, size_t n) {
    char* d = (char*)dst;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

kstatus_t copy_from_user_checked(void *dst, uintptr_t src, size_t len) {
    if (!dst || len == 0) return K_OK;
    if (!trap_user_range_valid(src, len)) return K_ERR_FAULT;

    if (src + len < src) return K_ERR_FAULT;

    internal_memcpy(dst, (const void *)src, len);
    return K_OK;
}

kstatus_t copy_to_user_checked(uintptr_t dst, const void *src, size_t len) {
    if (!src || len == 0) return K_OK;
    if (!trap_user_range_valid(dst, len)) return K_ERR_FAULT;

    if (dst + len < dst) return K_ERR_FAULT;

    internal_memcpy((void *)dst, src, len);
    return K_OK;
}

kstatus_t copy_user_string_checked(char *dst, uintptr_t src, size_t max_len) {
    if (!dst || max_len == 0) return K_OK;

    if (!trap_user_range_valid(src, 1)) return K_ERR_FAULT;

    size_t len = 0;
    const char *s = (const char *)src;
    while (len < max_len) {
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
