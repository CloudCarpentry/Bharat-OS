#include <bharat/libc/config.h>
#include "../core/memory/memory_backend.h"

static void *gpr_copy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n >= 8u) {
        d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=s[3];
        d[4]=s[4]; d[5]=s[5]; d[6]=s[6]; d[7]=s[7];
        d += 8; s += 8; n -= 8u;
    }
    while (n-- != 0u) *d++ = *s++;
    return dst;
}

static void *gpr_move(void *dst, const void *src, size_t n) {
    const uintptr_t da = (uintptr_t)dst;
    const uintptr_t sa = (uintptr_t)src;
    if (n == 0u || da == sa) return dst;
    if (da < sa || da - sa >= n) return gpr_copy(dst, src, n);
    unsigned char *d = (unsigned char *)dst + n;
    const unsigned char *s = (const unsigned char *)src + n;
    while (n-- != 0u) *--d = *--s;
    return dst;
}

static void *gpr_set(void *dst, int c, size_t n) {
    unsigned char *d = dst;
    const unsigned char v = (unsigned char)c;
    while (n >= 8u) {
        d[0]=v; d[1]=v; d[2]=v; d[3]=v; d[4]=v; d[5]=v; d[6]=v; d[7]=v;
        d += 8; n -= 8u;
    }
    while (n-- != 0u) *d++ = v;
    return dst;
}

static int gpr_compare(const void *lhs, const void *rhs, size_t n) {
    const unsigned char *a = lhs;
    const unsigned char *b = rhs;
    while (n-- != 0u) {
        const unsigned av = *a++;
        const unsigned bv = *b++;
        if (av != bv) return (int)av - (int)bv;
    }
    return 0;
}

static const bh_libc_memops_backend_t gpr_backend = {
    gpr_copy, gpr_move, gpr_set, gpr_compare
};

#if defined(BH_LIBC_ARCH_X86_64)
static void *erms_copy(void *dst, const void *src, size_t n) {
    void *ret = dst;
    __asm__ __volatile__("rep movsb" : "+D"(dst), "+S"(src), "+c"(n) : : "memory");
    return ret;
}
static void *erms_set(void *dst, int c, size_t n) {
    void *ret = dst;
    __asm__ __volatile__("rep stosb" : "+D"(dst), "+c"(n) : "a"(c) : "memory");
    return ret;
}
static const bh_libc_memops_backend_t erms_backend = {
    erms_copy, gpr_move, erms_set, gpr_compare
};
#endif

const bh_libc_memops_backend_t *
bh_libc_arch_memops_select(const bharat_cpu_features_v1_t *features) {
#if defined(BH_LIBC_ARCH_X86_64)
    if ((features->feature_words[0] &
         (1ULL << BHARAT_CPU_FEATURE_FAST_STRING)) != 0u) {
        return &erms_backend;
    }
#else
    (void)features;
#endif
    return &gpr_backend;
}
