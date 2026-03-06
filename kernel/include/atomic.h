#ifndef BHARAT_ATOMIC_H
#define BHARAT_ATOMIC_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Universal Atomic API
 * Wraps compiler-specific intrinsic atomic operations to provide
 * a unified lock-free API across different architectures.
 */

// Memory barrier to enforce ordering
#define smp_mb() __sync_synchronize()

typedef struct {
    volatile int value;
} atomic_t;

typedef struct {
    volatile uint64_t value;
} atomic64_t;

// 32-bit atomic operations
static inline void atomic_set(atomic_t *v, int i) {
    __sync_lock_test_and_set(&v->value, i);
}

static inline int atomic_get(const atomic_t *v) {
    return __sync_fetch_and_add((int*)&v->value, 0);
}

static inline int atomic_add_return(atomic_t *v, int i) {
    return __sync_add_and_fetch(&v->value, i);
}

static inline int atomic_sub_return(atomic_t *v, int i) {
    return __sync_sub_and_fetch(&v->value, i);
}

static inline void atomic_add(atomic_t *v, int i) {
    __sync_fetch_and_add(&v->value, i);
}

static inline void atomic_sub(atomic_t *v, int i) {
    __sync_fetch_and_sub(&v->value, i);
}

// 64-bit atomic operations
static inline void atomic64_set(atomic64_t *v, uint64_t i) {
    __sync_lock_test_and_set(&v->value, i);
}

static inline uint64_t atomic64_get(const atomic64_t *v) {
    return __sync_fetch_and_add((uint64_t*)&v->value, 0);
}

static inline void atomic64_add(atomic64_t *v, uint64_t i) {
    __sync_fetch_and_add(&v->value, i);
}

static inline void atomic64_sub(atomic64_t *v, uint64_t i) {
    __sync_fetch_and_sub(&v->value, i);
}

static inline uint64_t atomic64_fetch_and_or(volatile uint64_t *ptr, uint64_t mask) {
    return __sync_fetch_and_or(ptr, mask);
}

static inline uint64_t atomic64_fetch_and_and(volatile uint64_t *ptr, uint64_t mask) {
    return __sync_fetch_and_and(ptr, mask);
}

static inline uint64_t atomic64_fetch_and_add_ptr(volatile uint64_t *ptr, uint64_t val) {
    return __sync_fetch_and_add(ptr, val);
}

static inline uint64_t atomic64_fetch_and_sub_ptr(volatile uint64_t *ptr, uint64_t val) {
    return __sync_fetch_and_sub(ptr, val);
}

// 16-bit atomic operations for reference counts
static inline uint16_t atomic16_fetch_and_add(volatile uint16_t *ptr, uint16_t val) {
    return __sync_fetch_and_add(ptr, val);
}

static inline uint16_t atomic16_fetch_and_sub(volatile uint16_t *ptr, uint16_t val) {
    return __sync_fetch_and_sub(ptr, val);
}

// 32-bit unsigned
static inline uint32_t atomic32_fetch_and_add(volatile uint32_t *ptr, uint32_t val) {
    return __sync_fetch_and_add(ptr, val);
}

static inline uint32_t atomic32_fetch_and_sub(volatile uint32_t *ptr, uint32_t val) {
    return __sync_fetch_and_sub(ptr, val);
}

#endif // BHARAT_ATOMIC_H
