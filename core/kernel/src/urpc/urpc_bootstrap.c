#include "urpc/urpc_bootstrap.h"
#include <stddef.h>
#include <stdatomic.h>

// A global pool of per-core URPC channels connecting the Boot CPU (core 0)
// with each secondary core. For a full multikernel, this would be an NxN matrix
// or dynamically routed tree, but for Phase 1 bootstrap, N:1 (BSP -> Secondary) is the baseline.
static urpc_channel_t g_urpc_channels[BHARAT_MAX_CPUS];

static inline void init_ring(urpc_ring_t* ring) {
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);
    for (int i = 0; i < URPC_RING_SIZE; i++) {
        ring->buffer[i] = 0;
    }
}

kstatus_t urpc_init_global(void) {
    // Boot CPU initializes the structures
    for (uint32_t i = 0; i < BHARAT_MAX_CPUS; i++) {
        init_ring(&g_urpc_channels[i].tx_ring);
        init_ring(&g_urpc_channels[i].rx_ring);
        g_urpc_channels[i].is_bound = false;
    }
    return K_OK;
}

kstatus_t urpc_bootstrap_core(uint32_t core_id) {
    if (core_id == 0) return K_OK; // Boot CPU doesn't bind to itself

    if (core_id < BHARAT_MAX_CPUS) {
        // "Bind" endpoints by verifying memory access and state
        if (atomic_load_explicit(&g_urpc_channels[core_id].tx_ring.head, memory_order_relaxed) == 0 &&
            atomic_load_explicit(&g_urpc_channels[core_id].rx_ring.head, memory_order_relaxed) == 0) {
            g_urpc_channels[core_id].is_bound = true;
            return K_OK;
        }
    }
    return K_ERR_BAD_STATE;
}

void urpc_mark_ready(uint32_t core_id) {
    if (core_id < BHARAT_MAX_CPUS) {
        // Assume readiness implies binding was successful
        g_urpc_channels[core_id].is_bound = true;
    }
}

int urpc_is_ready(uint32_t core_id) {
    if (core_id < BHARAT_MAX_CPUS) {
        return g_urpc_channels[core_id].is_bound ? 1 : 0;
    }
    return 0;
}

// Minimal Lockless Queue (SPMC/MPSC depends on usage, currently Single Producer Single Consumer per ring)
kstatus_t urpc_bootstrap_send(uint32_t target_core, uint64_t msg) {
    if (target_core >= BHARAT_MAX_CPUS || !g_urpc_channels[target_core].is_bound) return K_ERR_NOT_FOUND;

    urpc_ring_t* ring = &g_urpc_channels[target_core].tx_ring;

    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % URPC_RING_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    // Check if full
    if (next_head == tail) {
        return K_ERR_BUSY;
    }

    ring->buffer[head] = msg;
    atomic_store_explicit(&ring->head, next_head, memory_order_release);

    return K_OK;
}

kstatus_t urpc_bootstrap_recv(uint32_t source_core, uint64_t* out_msg) {
    if (source_core >= BHARAT_MAX_CPUS || !g_urpc_channels[source_core].is_bound) return K_ERR_NOT_FOUND;
    if (out_msg == NULL) return K_ERR_INVALID_ARG;

    urpc_ring_t* ring = &g_urpc_channels[source_core].rx_ring;

    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    // Check if empty
    if (head == tail) {
        return K_ERR_NOT_FOUND;
    }

    *out_msg = ring->buffer[tail];
    uint32_t next_tail = (tail + 1) % URPC_RING_SIZE;
    atomic_store_explicit(&ring->tail, next_tail, memory_order_release);

    return K_OK;
}
