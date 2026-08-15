#define BH_JOIN_INNER(a, b) a##b
#define BH_JOIN(a, b) BH_JOIN_INNER(a, b)
#define BH_MEMOP(name) BH_JOIN(BH_JOIN(hal_##name##_, PREFIX), _gpr)

void *BH_MEMOP(memcpy)(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n >= 4u) {
        d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
        d += 4; s += 4; n -= 4u;
    }
    while (n-- != 0u) *d++ = *s++;
    return dst;
}

void *BH_MEMOP(memset)(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    const uint8_t v = (uint8_t)c;
    while (n >= 4u) {
        d[0] = v; d[1] = v; d[2] = v; d[3] = v;
        d += 4; n -= 4u;
    }
    while (n-- != 0u) *d++ = v;
    return dst;
}

void *BH_MEMOP(memmove)(void *dst, const void *src, size_t n) {
    const uintptr_t da = (uintptr_t)dst;
    const uintptr_t sa = (uintptr_t)src;
    if (n == 0u || da == sa) return dst;
    if (da < sa || da - sa >= n) return BH_MEMOP(memcpy)(dst, src, n);
    uint8_t *d = (uint8_t *)dst + n;
    const uint8_t *s = (const uint8_t *)src + n;
    while (n-- != 0u) *--d = *--s;
    return dst;
}

int BH_MEMOP(memcmp)(const void *lhs, const void *rhs, size_t n) {
    const uint8_t *a = lhs;
    const uint8_t *b = rhs;
    while (n-- != 0u) {
        const unsigned av = *a++;
        const unsigned bv = *b++;
        if (av != bv) return (int)av - (int)bv;
    }
    return 0;
}

#undef BH_MEMOP
#undef BH_JOIN
#undef BH_JOIN_INNER
#undef PREFIX
