#include <bharat/libc/memops.h>
#include <standard/string.h>

#include "memory_backend.h"

static void *generic_copy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n-- != 0u) *d++ = *s++;
    return dest;
}

static void *generic_move(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    const uintptr_t da = (uintptr_t)dest;
    const uintptr_t sa = (uintptr_t)src;
    if (n == 0u || da == sa) return dest;
    if (da < sa || da - sa >= n) return generic_copy(dest, src, n);
    d += n; s += n;
    while (n-- != 0u) *--d = *--s;
    return dest;
}

static void *generic_set(void *dest, int c, size_t n) {
    unsigned char *d = dest;
    while (n-- != 0u) *d++ = (unsigned char)c;
    return dest;
}

static int generic_compare(const void *lhs, const void *rhs, size_t n) {
    const unsigned char *a = lhs;
    const unsigned char *b = rhs;
    while (n-- != 0u) {
        const unsigned av = *a++;
        const unsigned bv = *b++;
        if (av != bv) return (int)av - (int)bv;
    }
    return 0;
}

static const bh_libc_memops_backend_t generic_backend = {
    generic_copy, generic_move, generic_set, generic_compare
};
static const bh_libc_memops_backend_t *selected_backend = &generic_backend;
/* CRT owns 0 -> 1 -> 2 publication; readers acquire the immutable pointer. */
static int resolver_state;

bool bh_libc_memops_init(const bharat_cpu_features_v1_t *features) {
    if (features == NULL || features->abi_version != BHARAT_CPU_FEATURES_ABI_V1 ||
        features->size < sizeof(*features)) return false;
    int expected = 0;
    if (!__atomic_compare_exchange_n(&resolver_state, &expected, 1, false,
                                     __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) return false;
    const bh_libc_memops_backend_t *candidate = bh_libc_arch_memops_select(features);
    if (candidate != NULL) __atomic_store_n(&selected_backend, candidate, __ATOMIC_RELEASE);
    __atomic_store_n(&resolver_state, 2, __ATOMIC_RELEASE);
    return true;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    return __atomic_load_n(&selected_backend, __ATOMIC_ACQUIRE)->copy(dest, src, n);
}
void *memmove(void *dest, const void *src, size_t n) {
    return __atomic_load_n(&selected_backend, __ATOMIC_ACQUIRE)->move(dest, src, n);
}
void *memset(void *dest, int c, size_t n) {
    return __atomic_load_n(&selected_backend, __ATOMIC_ACQUIRE)->set(dest, c, n);
}
int memcmp(const void *lhs, const void *rhs, size_t n) {
    return __atomic_load_n(&selected_backend, __ATOMIC_ACQUIRE)->compare(lhs, rhs, n);
}
