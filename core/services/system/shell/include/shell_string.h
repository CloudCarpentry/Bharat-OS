#ifndef BHARAT_SYSTEM_SHELL_STRING_H
#define BHARAT_SYSTEM_SHELL_STRING_H

#include <stddef.h>

static inline size_t shell_strlen(const char* s) {
    size_t n = 0;
    if (!s) {
        return 0;
    }
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

static inline int shell_strcmp(const char* a, const char* b) {
    size_t i = 0;
    if (!a || !b) {
        return (a == b) ? 0 : 1;
    }
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return (int)((unsigned char)a[i] - (unsigned char)b[i]);
        }
        ++i;
    }
    return (int)((unsigned char)a[i] - (unsigned char)b[i]);
}

static inline void shell_memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    size_t i;
    for (i = 0; i < n; ++i) {
        d[i] = s[i];
    }
}

#endif
