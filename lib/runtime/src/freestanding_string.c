#include <bharat/runtime/freestanding_string.h>

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while(n--) *d++ = *s++;
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while(s[len] != '\0') len++;
    return len;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *ret = dest;
    do {
        if (!n--) return ret;
    } while ((*dest++ = *src++));
    while (n--) *dest++ = 0;
    return ret;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    do {
        if (*s1 != *s2++) return (*(const unsigned char *)s1 - *(const unsigned char *)--s2);
        if (*s1++ == 0) break;
    } while (--n != 0);
    return 0;
}

// ── ARM AEABI Wrappers ───────────────────────────────────────────────────────
// These are required for ARM 32-bit builds when -nostdlib is used.
void __aeabi_memcpy(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memcpy4(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memcpy8(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memclr(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memclr4(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memclr8(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memset(void *dest, size_t n, int c) { memset(dest, c, n); }
void __aeabi_memset4(void *dest, size_t n, int c) { memset(dest, c, n); }
void __aeabi_memset8(void *dest, size_t n, int c) { memset(dest, c, n); }
