#ifndef BHARAT_ATOMIC_H
#define BHARAT_ATOMIC_H

#include "bharat/arch/types.h"
#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Universal Atomic API
 * Wraps compiler-specific intrinsic atomic operations to provide
 * a unified lock-free API across different architectures.
 */

// Memory barrier to enforce ordering
#define smp_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)

typedef struct {
    volatile int value;
} atomic_t;

typedef struct {
    volatile uint64_t value;
} atomic64_t;

// 32-bit atomic operations
static inline void atomic_set(atomic_t *v, int i) {
    __atomic_store_n(&v->value, i, __ATOMIC_RELEASE);
}

static inline int atomic_get(const atomic_t *v) {
    return __atomic_load_n(&v->value, __ATOMIC_ACQUIRE);
}

static inline int atomic_add_return(atomic_t *v, int i) {
    return __atomic_add_fetch(&v->value, i, __ATOMIC_SEQ_CST);
}

static inline int atomic_sub_return(atomic_t *v, int i) {
    return __atomic_sub_fetch(&v->value, i, __ATOMIC_SEQ_CST);
}

static inline void atomic_add(atomic_t *v, int i) {
    __atomic_fetch_add(&v->value, i, __ATOMIC_SEQ_CST);
}

static inline void atomic_sub(atomic_t *v, int i) {
    __atomic_fetch_sub(&v->value, i, __ATOMIC_SEQ_CST);
}

#if ARCH_WORD_BITS == 64
// 64-bit native atomic operations
static inline void atomic64_set(atomic64_t *v, uint64_t i) {
    __atomic_store_n(&v->value, i, __ATOMIC_RELEASE);
}

static inline uint64_t atomic64_get(const atomic64_t *v) {
    return __atomic_load_n(&v->value, __ATOMIC_ACQUIRE);
}

static inline void atomic64_add(atomic64_t *v, uint64_t i) {
    __atomic_fetch_add(&v->value, i, __ATOMIC_SEQ_CST);
}

static inline void atomic64_sub(atomic64_t *v, uint64_t i) {
    __atomic_fetch_sub(&v->value, i, __ATOMIC_SEQ_CST);
}

static inline uint64_t atomic64_fetch_and_or(volatile uint64_t *ptr, uint64_t mask) {
    return __atomic_fetch_or(ptr, mask, __ATOMIC_SEQ_CST);
}

static inline uint64_t atomic64_fetch_and_and(volatile uint64_t *ptr, uint64_t mask) {
    return __atomic_fetch_and(ptr, mask, __ATOMIC_SEQ_CST);
}

static inline uint64_t atomic64_fetch_and_add_ptr(volatile uint64_t *ptr, uint64_t val) {
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

static inline uint64_t atomic64_fetch_and_sub_ptr(volatile uint64_t *ptr, uint64_t val) {
    return __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST);
}
#else
// 32-bit fallback for 64-bit atomics.
// On 32-bit platforms, 64-bit atomic operations might not be lock-free and
// could require linking libatomic, which we don't have in bare-metal.
// For now, we wrap them in a global spinlock if ARCH_WORD_BITS == 32.
// Alternatively, architectures that support lock-free 64-bit atomics on 32-bit (like ARMv7 LPAE)
// can opt-in. But as a safe fallback, we use a global lock.

extern void arch_atomic64_lock(void);
extern void arch_atomic64_unlock(void);

static inline void atomic64_set(atomic64_t *v, uint64_t i) {
    arch_atomic64_lock();
    v->value = i;
    arch_atomic64_unlock();
}

static inline uint64_t atomic64_get(const atomic64_t *v) {
    arch_atomic64_lock();
    uint64_t val = v->value;
    arch_atomic64_unlock();
    return val;
}

static inline void atomic64_add(atomic64_t *v, uint64_t i) {
    arch_atomic64_lock();
    v->value += i;
    arch_atomic64_unlock();
}

static inline void atomic64_sub(atomic64_t *v, uint64_t i) {
    arch_atomic64_lock();
    v->value -= i;
    arch_atomic64_unlock();
}

static inline uint64_t atomic64_fetch_and_or(volatile uint64_t *ptr, uint64_t mask) {
    arch_atomic64_lock();
    uint64_t old = *ptr;
    *ptr |= mask;
    arch_atomic64_unlock();
    return old;
}

static inline uint64_t atomic64_fetch_and_and(volatile uint64_t *ptr, uint64_t mask) {
    arch_atomic64_lock();
    uint64_t old = *ptr;
    *ptr &= mask;
    arch_atomic64_unlock();
    return old;
}

static inline uint64_t atomic64_fetch_and_add_ptr(volatile uint64_t *ptr, uint64_t val) {
    arch_atomic64_lock();
    uint64_t old = *ptr;
    *ptr += val;
    arch_atomic64_unlock();
    return old;
}

static inline uint64_t atomic64_fetch_and_sub_ptr(volatile uint64_t *ptr, uint64_t val) {
    arch_atomic64_lock();
    uint64_t old = *ptr;
    *ptr -= val;
    arch_atomic64_unlock();
    return old;
}
#endif

// 16-bit atomic operations for reference counts
static inline uint16_t atomic16_fetch_and_add(volatile uint16_t *ptr, uint16_t val) {
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

static inline uint16_t atomic16_fetch_and_sub(volatile uint16_t *ptr, uint16_t val) {
    return __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST);
}

// 32-bit unsigned
static inline uint32_t atomic32_fetch_and_add(volatile uint32_t *ptr, uint32_t val) {
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

static inline uint32_t atomic32_fetch_and_sub(volatile uint32_t *ptr, uint32_t val) {
    return __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST);
}

#endif // BHARAT_ATOMIC_H
