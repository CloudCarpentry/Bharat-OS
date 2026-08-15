#include "ipc/mk_proto.h"
#include <stdbool.h>

#define BH_MK_MPSC_MAX_CAPACITY (1U << 31)

_Static_assert(BH_MK_LANE_CONTROL_CAP < BH_MK_MPSC_MAX_CAPACITY, "control lane must preserve modulo ordering");
_Static_assert(BH_MK_LANE_NORMAL_CAP < BH_MK_MPSC_MAX_CAPACITY, "normal lane must preserve modulo ordering");
_Static_assert(BH_MK_LANE_BULK_CAP < BH_MK_MPSC_MAX_CAPACITY, "bulk lane must preserve modulo ordering");

kstatus_t bh_mk_mpsc_ring_init(bh_mk_mpsc_ring_t *ring, bh_mk_ring_slot_t *slots, uint32_t capacity) {
    if (!ring || !slots || capacity == 0 || capacity >= BH_MK_MPSC_MAX_CAPACITY ||
        (capacity & (capacity - 1U)) != 0U) {
        return K_ERR_INVALID_ARG;
    }

    ring->slots = slots;
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    atomic_store_explicit(&ring->producer_head, 0, memory_order_relaxed);
    ring->consumer_tail = 0;
    atomic_store_explicit(&ring->available_credits, capacity, memory_order_relaxed);

    for (uint32_t i = 0; i < capacity; i++) {
        atomic_store_explicit(&slots[i].sequence, i, memory_order_relaxed);
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
    uint32_t pos = atomic_load_explicit(&ring->producer_head, memory_order_relaxed);
    uint32_t attempts = 0;
    for (;;) {
        bh_mk_ring_slot_t *slot = &ring->slots[pos & ring->mask];
        uint32_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        int32_t diff = (int32_t)(seq - pos);
        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&ring->producer_head, &pos, pos + 1U, memory_order_relaxed, memory_order_relaxed)) {
                // Success! We reserved slot at pos.
                slot->message = *msg;
                atomic_store_explicit(&slot->sequence, pos + 1U, memory_order_release);
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

    uint32_t pos = ring->consumer_tail;
    bh_mk_ring_slot_t *slot = &ring->slots[pos & ring->mask];
    uint32_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
    int32_t diff = (int32_t)(seq - (pos + 1U));

    if (diff == 0) {
        *out_msg = slot->message;
        // Mark slot ready for the next producer cycle
        atomic_store_explicit(&slot->sequence, pos + ring->capacity, memory_order_release);
        ring->consumer_tail = pos + 1U;

        // Replenish credit
        atomic_fetch_add_explicit(&ring->available_credits, 1, memory_order_release);
        return K_OK;
    } else {
        return K_ERR_AGAIN;
    }
}
