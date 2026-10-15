#include <stdint.h>
#include <stddef.h>
#include "lib/base/string.h"
void __aeabi_memcpy(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memcpy4(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memcpy8(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memclr(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memclr4(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memclr8(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memset(void *dest, size_t n, int c) { memset(dest, c, n); }
void __aeabi_memset4(void *dest, size_t n, int c) { memset(dest, c, n); }
void __aeabi_memset8(void *dest, size_t n, int c) { memset(dest, c, n); }

uint64_t __aeabi_uidivmod(unsigned int n, unsigned int d) {
    if (d == 0) return 0;
    unsigned int q = 0, r = 0;
    for (int i = 31; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1U << i); }
    }
    return ((uint64_t)r << 32) | q;
}

unsigned int __aeabi_uidiv(unsigned int n, unsigned int d) {
    return (unsigned int)__aeabi_uidivmod(n, d);
}

int __aeabi_idiv(int n, int d) {
    if (d == 0) return 0;
    int neg = (n < 0) ^ (d < 0);
    unsigned int un = (n < 0) ? -(unsigned int)n : (unsigned int)n;
    unsigned int ud = (d < 0) ? -(unsigned int)d : (unsigned int)d;
    unsigned int q = __aeabi_uidiv(un, ud);
    return neg ? -(int)q : (int)q;
}

int64_t __aeabi_idivmod(int n, int d) {
    if (d == 0) return 0;
    int neg_q = (n < 0) ^ (d < 0);
    int neg_r = (n < 0);
    unsigned int un = (n < 0) ? -(unsigned int)n : (unsigned int)n;
    unsigned int ud = (d < 0) ? -(unsigned int)d : (unsigned int)d;
    uint64_t res = __aeabi_uidivmod(un, ud);
    unsigned int q = (unsigned int)res;
    unsigned int r = (unsigned int)(res >> 32);
    int sq = neg_q ? -(int)q : (int)q;
    int sr = neg_r ? -(int)r : (int)r;
    return ((int64_t)sr << 32) | (uint32_t)sq;
}


uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1ULL << i); }
    }
    return q;
}
uint64_t __aeabi_uldivmod(uint64_t n, uint64_t d) { return __udivdi3(n, d); }
