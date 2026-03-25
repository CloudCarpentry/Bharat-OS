/*
 * kernel/src/lib/ds/urpc_ring.c
 * uRPC Ring implementation for inter-core communication.
 */

#include "lib/ds/urpc_ring.h"
#include <stddef.h>
#include <stdatomic.h>

/* Minimal memory ops as fallback */
static void urpc_memcpy(void *dst, const void *src, size_t n) {
    char *d = dst;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
}

void urpc_ring_init(urpc_ring_t *ring) {
    atomic_init((_Atomic uint32_t*)&ring->head, 0);
    atomic_init((_Atomic uint32_t*)&ring->tail, 0);
}

int urpc_ring_send(urpc_ring_t *ring, const void *msg) {
    uint32_t head, tail, next_tail;

    do {
        tail = atomic_load_explicit((_Atomic uint32_t*)&ring->tail, memory_order_acquire);
        head = atomic_load_explicit((_Atomic uint32_t*)&ring->head, memory_order_acquire);

        next_tail = (tail + 1) % (URPC_RING_SIZE * URPC_MSG_SIZE / URPC_MSG_SIZE);

        if (next_tail == head) {
            return -1; /* Ring is full */
        }
    } while (!atomic_compare_exchange_weak_explicit(
                (_Atomic uint32_t*)&ring->tail, &tail, next_tail,
                memory_order_release, memory_order_relaxed));

    urpc_memcpy(&ring->buffer[tail * URPC_MSG_SIZE], msg, URPC_MSG_SIZE);

    return 0;
}

int urpc_ring_recv(urpc_ring_t *ring, void *msg_out) {
    uint32_t head, tail, next_head;

    do {
        head = atomic_load_explicit((_Atomic uint32_t*)&ring->head, memory_order_acquire);
        tail = atomic_load_explicit((_Atomic uint32_t*)&ring->tail, memory_order_acquire);

        if (head == tail) {
            return -1; /* Ring is empty */
        }

        next_head = (head + 1) % (URPC_RING_SIZE * URPC_MSG_SIZE / URPC_MSG_SIZE);

    } while (!atomic_compare_exchange_weak_explicit(
                (_Atomic uint32_t*)&ring->head, &head, next_head,
                memory_order_release, memory_order_relaxed));

    urpc_memcpy(msg_out, &ring->buffer[head * URPC_MSG_SIZE], URPC_MSG_SIZE);

    return 0;
}
