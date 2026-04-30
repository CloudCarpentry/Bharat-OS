#include "bharat/kernel/ds/bh_mpsc_queue.h"

static inline bool is_power_of_two(uint32_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

kstatus_t bh_mpsc_queue_init(bh_mpsc_queue_t *q, bh_mpsc_slot_t *slots, uint32_t capacity) {
    if (!q || !slots || capacity < 2 || !is_power_of_two(capacity)) {
        return K_ERR_INVALID_ARG;
    }

    q->slots = slots;
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->head = 0;
    q->tail = 0;

    for (uint32_t i = 0; i < capacity; i++) {
        q->slots[i].seq = i;
        q->slots[i].value = NULL;
    }

    return K_OK;
}

kstatus_t bh_mpsc_queue_push(bh_mpsc_queue_t *q, void *value) {
    bh_mpsc_slot_t *slot;
    uint64_t pos = q->head;

    while (true) {
        slot = &q->slots[pos & q->mask];
        uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            /* Slot is ready to be written */
            if (__atomic_compare_exchange_n(&q->head, &pos, pos + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
                break;
            }
        } else if (diff < 0) {
            /* Queue is full */
            return K_ERR_AGAIN;
        } else {
            /* Another producer beat us, or we are looking at an old 'pos' */
            pos = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
        }
    }

    slot->value = value;
    __atomic_store_n(&slot->seq, pos + 1, __ATOMIC_RELEASE);

    return K_OK;
}

kstatus_t bh_mpsc_queue_pop(bh_mpsc_queue_t *q, void **out_value) {
    bh_mpsc_slot_t *slot;
    uint64_t pos = q->tail;

    slot = &q->slots[pos & q->mask];
    uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
    int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

    if (diff == 0) {
        /* Slot has data ready to be read */
        q->tail = pos + 1;
        if (out_value) {
            *out_value = slot->value;
        }
        __atomic_store_n(&slot->seq, pos + q->mask + 1, __ATOMIC_RELEASE);
        return K_OK;
    }

    /* Queue is empty or data is being written by producer */
    return K_ERR_AGAIN;
}

bool bh_mpsc_queue_empty(const bh_mpsc_queue_t *q) {
    if (!q) return true;
    uint64_t head = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
    return q->tail == head;
}

uint32_t bh_mpsc_queue_capacity(const bh_mpsc_queue_t *q) {
    return q ? q->capacity : 0;
}
