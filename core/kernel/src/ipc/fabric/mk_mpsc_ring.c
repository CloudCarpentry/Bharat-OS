#include "ipc/mk_proto.h"
#include <stdbool.h>

kstatus_t bh_mk_mpsc_ring_init(bh_mk_mpsc_ring_t *ring, bh_mk_ring_slot_t *slots, uint32_t capacity) {
    if (!ring || !slots || capacity == 0 || (capacity & (capacity - 1)) != 0) {
        return K_ERR_INVALID_ARG;
    }

    ring->slots = slots;
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    atomic_store_explicit(&ring->producer_head, 0, memory_order_relaxed);
    ring->consumer_tail = 0;
    atomic_store_explicit(&ring->available_credits, capacity, memory_order_relaxed);

    for (uint32_t i = 0; i < capacity; i++) {
        atomic_store_explicit(&slots[i].sequence, (uint64_t)i, memory_order_relaxed);
        __builtin_memset(&slots[i].message, 0, sizeof(bh_mk_wire_message_t));
    }

    return K_OK;
}

kstatus_t bh_mk_mpsc_ring_enqueue(bh_mk_mpsc_ring_t *ring, const bh_mk_wire_message_t *msg) {
    if (!ring || !msg) {
        return K_ERR_INVALID_ARG;
    }

    // 1. Consume one credit
    uint32_t creds = atomic_load_explicit(&ring->available_credits, memory_order_relaxed);
    for (;;) {
        if (creds == 0) {
            return K_ERR_WOULD_BLOCK;
        }
        if (atomic_compare_exchange_weak_explicit(&ring->available_credits, &creds, creds - 1, memory_order_acquire, memory_order_relaxed)) {
            break;
        }
    }

    // 2. Reserve ticket (producer_head)
    uint64_t pos = atomic_load_explicit(&ring->producer_head, memory_order_relaxed);
    uint32_t attempts = 0;
    for (;;) {
        bh_mk_ring_slot_t *slot = &ring->slots[pos & ring->mask];
        uint64_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)pos;
        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&ring->producer_head, &pos, pos + 1, memory_order_relaxed, memory_order_relaxed)) {
                // Success! We reserved slot at pos.
                slot->message = *msg;
                atomic_store_explicit(&slot->sequence, pos + 1, memory_order_release);
                return K_OK;
            }
        } else if (diff < 0) {
            attempts++;
            if (attempts > 2000) {
                // Return credit before leaving!
                atomic_fetch_add_explicit(&ring->available_credits, 1, memory_order_release);
                return K_ERR_WOULD_BLOCK;
            }
            pos = atomic_load_explicit(&ring->producer_head, memory_order_relaxed);
        } else {
            pos = atomic_load_explicit(&ring->producer_head, memory_order_relaxed);
        }
    }
}

kstatus_t bh_mk_mpsc_ring_dequeue(bh_mk_mpsc_ring_t *ring, bh_mk_wire_message_t *out_msg) {
    if (!ring || !out_msg) {
        return K_ERR_INVALID_ARG;
    }

    uint64_t pos = ring->consumer_tail;
    bh_mk_ring_slot_t *slot = &ring->slots[pos & ring->mask];
    uint64_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

    if (diff == 0) {
        *out_msg = slot->message;
        // Mark slot ready for the next producer cycle
        atomic_store_explicit(&slot->sequence, pos + ring->capacity, memory_order_release);
        ring->consumer_tail = pos + 1;

        // Replenish credit
        atomic_fetch_add_explicit(&ring->available_credits, 1, memory_order_release);
        return K_OK;
    } else {
        return K_ERR_AGAIN;
    }
}
