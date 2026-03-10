#ifndef BHARAT_URPC_BOOTSTRAP_H
#define BHARAT_URPC_BOOTSTRAP_H

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_boot.h"

#define URPC_RING_SIZE 256

// A minimal lockless ring buffer for cross-core messaging
typedef struct {
    volatile uint32_t head;
    volatile uint32_t tail;
    uint64_t buffer[URPC_RING_SIZE];
} urpc_ring_t;

// Each core pair has a channel (Tx from core A -> core B, Rx from core B -> core A)
typedef struct {
    urpc_ring_t tx_ring;
    urpc_ring_t rx_ring;
    bool is_bound;
} urpc_channel_t;

// Boot core initializes the global URPC pool
int urpc_init_global(void);

// Secondary core binds to its channel
int urpc_bootstrap_core(uint32_t core_id);

// Mark channel as ready for traffic
void urpc_mark_ready(uint32_t core_id);

// Check if a core's channel is ready
int urpc_is_ready(uint32_t core_id);

// Send a basic message (non-blocking, returns -1 if full)
int urpc_send(uint32_t target_core, uint64_t msg);

// Receive a basic message (non-blocking, returns -1 if empty)
int urpc_recv(uint32_t source_core, uint64_t* out_msg);

#endif // BHARAT_URPC_BOOTSTRAP_H
