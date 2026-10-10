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
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);
}

int urpc_ring_send(urpc_ring_t *ring, const void *msg) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % URPC_RING_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        return -1; /* Ring is full */
    }

    urpc_memcpy(&ring->buffer[head * URPC_MSG_SIZE], msg, URPC_MSG_SIZE);

    atomic_store_explicit(&ring->head, next_head, memory_order_release);

    return 0;
}

int urpc_ring_recv(urpc_ring_t *ring, void *msg_out) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (tail == head) {
        return -1; /* Ring is empty */
    }

    urpc_memcpy(msg_out, &ring->buffer[tail * URPC_MSG_SIZE], URPC_MSG_SIZE);

    uint32_t next_tail = (tail + 1) % URPC_RING_SIZE;
    atomic_store_explicit(&ring->tail, next_tail, memory_order_release);

    return 0;
}
