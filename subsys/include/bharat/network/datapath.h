#ifndef BHARAT_NETWORK_DATAPATH_H
#define BHARAT_NETWORK_DATAPATH_H

#include "types.h"

/* Data-plane-facing contracts */

typedef struct {
    void *ring_base;
    uint32_t ring_size;
    uint16_t descriptor_size;
} bnet_ring_handoff_t;

/* Buffer Ownership States */
typedef enum {
    BNET_BUF_OWN_CLIENT = 0,
    BNET_BUF_OWN_STACK,
    BNET_BUF_OWN_HW
} bnet_buffer_owner_t;

/* Queue Capabilities */
#define BNET_QUEUE_CAP_ZERO_COPY  (1 << 0)
#define BNET_QUEUE_CAP_OFFLOAD_CS (1 << 1)
#define BNET_QUEUE_CAP_OFFLOAD_TS (1 << 2)

/* Data-plane API */
int bnet_dp_queue_register(bnet_interface_id_t iface_id, bnet_queue_id_t q_id, const bnet_ring_handoff_t *handoff);
int bnet_dp_queue_unregister(bnet_interface_id_t iface_id, bnet_queue_id_t q_id);

int bnet_dp_notify_tx(bnet_interface_id_t iface_id, bnet_queue_id_t q_id);
int bnet_dp_poll_rx(bnet_interface_id_t iface_id, bnet_queue_id_t q_id, uint32_t count, void **out_buffers);

#endif // BHARAT_NETWORK_DATAPATH_H
