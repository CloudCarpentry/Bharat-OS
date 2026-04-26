#include <bharat/kernel/ipc/bh_core_msgq.h>

void bh_core_msgq_init(bh_core_msgq_t *q, void *storage, uint32_t capacity) {
    bh_ring_init(&q->ring, storage, capacity, sizeof(bh_core_msg_t));
}

kstatus_t bh_core_msgq_push(bh_core_msgq_t *q, const bh_core_msg_t *msg) {
    return bh_ring_push(&q->ring, msg);
}

kstatus_t bh_core_msgq_pop(bh_core_msgq_t *q, bh_core_msg_t *out_msg) {
    return bh_ring_pop(&q->ring, out_msg);
}
