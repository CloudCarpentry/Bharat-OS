#include "virtqueue.h"

void bh_virtqueue_init(bh_virtqueue_t *vq,
                       uint32_t queue_size,
                       bh_virtq_desc_t *desc,
                       bh_virtq_avail_t *avail,
                       bh_virtq_used_t *used) {
    if (!vq || !desc || !avail || !used || queue_size == 0 || queue_size > VIRTIO_RING_SIZE) {
        return;
    }

    __builtin_memset(vq, 0, sizeof(*vq));
    vq->queue_size = queue_size;
    vq->desc = desc;
    vq->avail = avail;
    vq->used = used;

    __builtin_memset(desc, 0, sizeof(*desc) * queue_size);
    __builtin_memset(avail, 0, sizeof(*avail));
    __builtin_memset(used, 0, sizeof(*used));

    // Link free list
    for (uint32_t i = 0; i < queue_size - 1; i++) {
        desc[i].next = (uint16_t)(i + 1);
    }
    desc[queue_size - 1].next = 0xFFFFU;

    vq->free_head = 0;
    vq->num_free = (uint16_t)queue_size;
    vq->last_used_idx = 0;
}

int bh_virtqueue_add_rx_buffer(bh_virtqueue_t *vq, void *buf, uint32_t len, uint16_t *out_desc_idx) {
    if (!vq || vq->num_free == 0) {
        return -1;
    }

    uint16_t desc_idx = vq->free_head;
    vq->free_head = vq->desc[desc_idx].next;
    vq->num_free--;

    vq->desc[desc_idx].addr = (uint64_t)(uintptr_t)buf;
    vq->desc[desc_idx].len = len;
    vq->desc[desc_idx].flags = VIRTQ_DESC_F_WRITE;
    vq->desc[desc_idx].next = 0xFFFFU;

    // Add to available ring with release memory barrier semantics
    uint16_t avail_idx = vq->avail->idx;
    vq->avail->ring[avail_idx % vq->queue_size] = desc_idx;
    __atomic_store_n(&vq->avail->idx, (uint16_t)(avail_idx + 1), __ATOMIC_RELEASE);

    if (out_desc_idx) {
        *out_desc_idx = desc_idx;
    }

    return 0;
}

bool bh_virtqueue_poll_used(bh_virtqueue_t *vq, uint16_t *out_desc_idx, uint32_t *out_len) {
    if (!vq) return false;

    // Read the host index with acquire memory barrier semantics
    uint16_t used_idx = __atomic_load_n(&vq->used->idx, __ATOMIC_ACQUIRE);
    if (used_idx == vq->last_used_idx) {
        return false;
    }

    uint32_t ring_slot = vq->last_used_idx % vq->queue_size;
    bh_virtq_used_elem_t *elem = &vq->used->ring[ring_slot];

    if (out_desc_idx) {
        *out_desc_idx = (uint16_t)elem->id;
    }
    if (out_len) {
        *out_len = elem->len;
    }

    vq->last_used_idx++;
    return true;
}

void bh_virtqueue_free_descriptor(bh_virtqueue_t *vq, uint16_t desc_idx) {
    if (!vq || desc_idx >= vq->queue_size) {
        return;
    }

    vq->desc[desc_idx].next = vq->free_head;
    vq->free_head = desc_idx;
    vq->num_free++;
}
