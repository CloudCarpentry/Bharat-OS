// kernel/include/spinlock.h
#ifndef BHARAT_SPINLOCK_H
#define BHARAT_SPINLOCK_H

#include "atomic.h"

typedef struct {
    atomic_t locked;
} spinlock_t;

static inline void spin_lock_init(spinlock_t* lock) {
    atomic_set(&lock->locked, 0);
}

static inline void spin_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->locked.value, 1)) {
        // Pause instruction to prevent semiconductor pipeline stalls
        // and reduce localized thermal power consumption
        __asm__ volatile("pause" ::: "memory");
    }
}

static inline void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(&lock->locked.value);
}

#endif // BHARAT_SPINLOCK_H
