#include "atomic.h"
#include "spinlock.h"
#include <stdbool.h>

#if ARCH_WORD_BITS == 32
static spinlock_t g_atomic64_lock;
static bool g_atomic64_lock_init = false;

void arch_atomic64_lock(void) {
    if (!g_atomic64_lock_init) {
        spin_lock_init(&g_atomic64_lock);
        g_atomic64_lock_init = true;
    }
    spin_lock(&g_atomic64_lock);
}

void arch_atomic64_unlock(void) {
    spin_unlock(&g_atomic64_lock);
}

uint64_t __atomic_fetch_add_8(volatile void *ptr, uint64_t val, int memorder) {
    (void)memorder;
    arch_atomic64_lock();
    volatile uint64_t *p = (volatile uint64_t *)ptr;
    uint64_t old = *p;
    *p = old + val;
    arch_atomic64_unlock();
    return old;
}

uint64_t __atomic_load_8(const volatile void *ptr, int memorder) {
    (void)memorder;
    arch_atomic64_lock();
    const volatile uint64_t *p = (const volatile uint64_t *)ptr;
    uint64_t v = *p;
    arch_atomic64_unlock();
    return v;
}

uint64_t __atomic_fetch_or_8(volatile void *ptr, uint64_t val, int memorder) {
    (void)memorder;
    arch_atomic64_lock();
    volatile uint64_t *p = (volatile uint64_t *)ptr;
    uint64_t old = *p;
    *p = old | val;
    arch_atomic64_unlock();
    return old;
}

uint64_t __atomic_fetch_and_8(volatile void *ptr, uint64_t val, int memorder) {
    (void)memorder;
    arch_atomic64_lock();
    volatile uint64_t *p = (volatile uint64_t *)ptr;
    uint64_t old = *p;
    *p = old & val;
    arch_atomic64_unlock();
    return old;
}

uint64_t __umoddi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; --i) {
        r = (r << 1) | ((n >> i) & 1ULL);
        if (r >= d) {
            r -= d;
        }
    }
    return r;
}
#endif
