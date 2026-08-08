#ifndef BHARAT_SPINLOCK_H
#define BHARAT_SPINLOCK_H

#include "atomic.h"
#include "arch/cpu_relax.h"

typedef struct {
    atomic_t locked;
} spinlock_t;

static inline void spin_lock_init(spinlock_t* lock) {
    atomic_set(&lock->locked, 0);
}

static inline void spin_lock(spinlock_t* lock) {
    while (__atomic_test_and_set(&lock->locked.value, __ATOMIC_ACQUIRE)) {
        // Architecture-specific wait hint to reduce contention/power.
        arch_cpu_relax();
    }
}

static inline void spin_unlock(spinlock_t* lock) {
    __atomic_clear(&lock->locked.value, __ATOMIC_RELEASE);
}

#endif // BHARAT_SPINLOCK_H
