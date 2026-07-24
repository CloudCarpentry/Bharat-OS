/*
 * kernel/src/lib/sync/mcs_lock.c
 * MCS Lock - Scalable mutual exclusion lock for multikernel.
 */

#include "lib/sync/mcs_lock.h"
#include <stdatomic.h>
#include <stddef.h>

void mcs_lock_acquire(mcs_lock_t *lock, mcs_qnode_t *node) {
    node->next = NULL;
    node->locked = true;

    mcs_qnode_t *prev = atomic_exchange_explicit((_Atomic(mcs_qnode_t *)*)lock, node, memory_order_acquire);

    if (prev != NULL) {
        atomic_store_explicit((_Atomic(mcs_qnode_t *)*)&prev->next, node, memory_order_release);
        while (atomic_load_explicit((_Atomic bool*)&node->locked, memory_order_acquire)) {
            /* Spin */
        }
    }
}

void mcs_lock_release(mcs_lock_t *lock, mcs_qnode_t *node) {
    if (atomic_load_explicit((_Atomic(mcs_qnode_t *)*)&node->next, memory_order_acquire) == NULL) {
        mcs_qnode_t *expected = node;
        if (atomic_compare_exchange_strong_explicit((_Atomic(mcs_qnode_t *)*)lock, &expected, NULL, memory_order_release, memory_order_relaxed)) {
            return;
        }

        while (atomic_load_explicit((_Atomic(mcs_qnode_t *)*)&node->next, memory_order_acquire) == NULL) {
            /* Spin waiting for the next node to link itself */
        }
    }

    atomic_store_explicit((_Atomic bool*)&node->next->locked, false, memory_order_release);
}
