#include "bharat/kernel/ds/bh_mpsc_queue.h"

static inline bool is_power_of_two(uint32_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

/*
 * Sequence comparison uses modulo-2^32 arithmetic.
 * Queue capacity is constrained below 2^31 so occupied/free
 * positions remain unambiguous under signed half-range comparison.
 */

kstatus_t bh_mpsc_queue_init(bh_mpsc_queue_t *q, bh_mpsc_slot_t *slots, uint32_t capacity) {
    if (!q ||
        !slots ||
        capacity < 2U ||
        !is_power_of_two(capacity) ||
        capacity > BH_MPSC_MAX_CAPACITY) {
        return K_ERR_INVALID_ARG;
    }

    q->slots = slots;
    q->capacity = capacity;
    q->mask = capacity - 1;
    atomic32_store_relaxed(&q->head, 0);
    q->tail = 0;

    for (uint32_t i = 0; i < capacity; i++) {
        atomic32_store_relaxed(&q->slots[i].seq, i);
        q->slots[i].value = NULL;
    }

    return K_OK;
}

kstatus_t bh_mpsc_queue_push(bh_mpsc_queue_t *q, void *value) {
    if (!q || !q->slots) {
        return K_ERR_INVALID_ARG;
    }

    bh_mpsc_slot_t *slot;
    uint32_t pos = atomic32_load_relaxed(&q->head);

    while (true) {
        slot = &q->slots[pos & q->mask];
        uint32_t seq = atomic32_load_acquire(&slot->seq);
        int32_t diff = (int32_t)(seq - pos);

        if (diff == 0) {
            /* Slot is ready to be written */
            uint32_t expected = pos;
            if (atomic32_compare_exchange_relaxed(&q->head, &expected, pos + 1U)) {
                break;
            }
            /* CAS failed, 'expected' contains the updated head */
            pos = expected;
        } else if (diff < 0) {
            /* Queue is full */
            return K_ERR_AGAIN;
        } else {
            /* Another producer beat us, or we are looking at an old 'pos' */
            pos = atomic32_load_relaxed(&q->head);
        }
    }

    slot->value = value;
    atomic32_store_release(&slot->seq, pos + 1U);

    return K_OK;
}

kstatus_t bh_mpsc_queue_pop(bh_mpsc_queue_t *q, void **out_value) {
    if (!q || !q->slots) {
        return K_ERR_INVALID_ARG;
    }

    bh_mpsc_slot_t *slot;
    uint32_t pos = q->tail;

    slot = &q->slots[pos & q->mask];
    uint32_t seq = atomic32_load_acquire(&slot->seq);
    int32_t diff = (int32_t)(seq - (pos + 1U));

    if (diff == 0) {
        /* Slot has data ready to be read */
        q->tail = pos + 1U;
        if (out_value) {
            *out_value = slot->value;
        }
        atomic32_store_release(&slot->seq, pos + q->capacity);
        return K_OK;
    }

    /* Queue is empty or data is being written by producer */
    return K_ERR_AGAIN;
}

bool bh_mpsc_queue_empty(const bh_mpsc_queue_t *q) {
    if (!q) return true;
    uint32_t head = atomic32_load_relaxed(&q->head);
    return q->tail == head;
}

uint32_t bh_mpsc_queue_capacity(const bh_mpsc_queue_t *q) {
    return q ? q->capacity : 0;
}
