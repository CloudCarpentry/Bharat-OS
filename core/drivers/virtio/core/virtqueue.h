#ifndef BHARAT_VIRTIO_CORE_VIRTQUEUE_H
#define BHARAT_VIRTIO_CORE_VIRTQUEUE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VIRTIO_RING_SIZE 64

#define VIRTQ_DESC_F_NEXT  1U
#define VIRTQ_DESC_F_WRITE 2U

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} bh_virtq_desc_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_RING_SIZE];
    uint16_t used_event;
} bh_virtq_avail_t;

typedef struct {
    uint32_t id;
    uint32_t len;
} bh_virtq_used_elem_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    bh_virtq_used_elem_t ring[VIRTIO_RING_SIZE];
    uint16_t avail_event;
} bh_virtq_used_t;

typedef struct {
    uint32_t queue_size;
    bh_virtq_desc_t *desc;
    bh_virtq_avail_t *avail;
    bh_virtq_used_t *used;

    uint16_t last_used_idx;
    uint16_t free_head;
    uint16_t num_free;
} bh_virtqueue_t;

/**
 * Initializes a virtqueue using the provided memory segments.
 */
void bh_virtqueue_init(bh_virtqueue_t *vq,
                       uint32_t queue_size,
                       bh_virtq_desc_t *desc,
                       bh_virtq_avail_t *avail,
                       bh_virtq_used_t *used);

/**
 * Adds a host-writable (receive) buffer to the virtqueue.
 */
int bh_virtqueue_add_rx_buffer(bh_virtqueue_t *vq, void *buf, uint32_t len, uint16_t *out_desc_idx);

/**
 * Polls the used ring for any newly completed descriptors.
 */
bool bh_virtqueue_poll_used(bh_virtqueue_t *vq, uint16_t *out_desc_idx, uint32_t *out_len);

/**
 * Releases/frees a descriptor chain back to the virtqueue.
 */
void bh_virtqueue_free_descriptor(bh_virtqueue_t *vq, uint16_t desc_idx);

#endif // BHARAT_VIRTIO_CORE_VIRTQUEUE_H
