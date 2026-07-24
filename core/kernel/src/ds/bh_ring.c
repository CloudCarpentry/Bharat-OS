#include <bharat/kernel/ds/bh_ring.h>
#include <kernel/status.h>
#include <lib/base/string.h>

void bh_ring_init(bh_ring_t *ring, void *storage, uint32_t capacity, uint32_t element_size) {
    ring->storage = (uint8_t *)storage;
    ring->capacity = capacity;
    ring->element_size = element_size;
    ring->head = 0;
    ring->tail = 0;
}

bool bh_ring_is_empty(const bh_ring_t *ring) {
    return ring->head == ring->tail;
}

bool bh_ring_is_full(const bh_ring_t *ring) {
    return ((ring->head + 1) % ring->capacity) == ring->tail;
}

kstatus_t bh_ring_push(bh_ring_t *ring, const void *data) {
    if (bh_ring_is_full(ring)) {
        return K_ERR_IPC_QUEUE_FULL;
    }

    memcpy(ring->storage + (ring->head * ring->element_size), data, ring->element_size);
    ring->head = (ring->head + 1) % ring->capacity;
    return K_OK;
}

kstatus_t bh_ring_pop(bh_ring_t *ring, void *data) {
    if (bh_ring_is_empty(ring)) {
        return K_ERR_NOT_FOUND;
    }

    if (data) {
        memcpy(data, ring->storage + (ring->tail * ring->element_size), ring->element_size);
    }
    ring->tail = (ring->tail + 1) % ring->capacity;
    return K_OK;
}
