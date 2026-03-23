#include "atomic.h"
#include "spinlock.h"

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
#endif
