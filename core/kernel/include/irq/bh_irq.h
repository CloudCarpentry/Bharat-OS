#ifndef BHARAT_IRQ_H
#define BHARAT_IRQ_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/status.h"
#include "irq/bh_irq_types.h"

// --- Handle Packing ---
static inline bh_irq_handle_t bh_irq_make_handle(uint16_t slot, uint32_t generation, uint8_t domain_id, uint8_t type) {
    return ((uint64_t)slot) |
           (((uint64_t)generation) << 16) |
           (((uint64_t)domain_id) << 48) |
           (((uint64_t)type) << 56);
}

static inline uint16_t bh_irq_handle_slot(bh_irq_handle_t h) {
    return (uint16_t)(h & 0xFFFFULL);
}

static inline uint32_t bh_irq_handle_generation(bh_irq_handle_t h) {
    return (uint32_t)((h >> 16) & 0xFFFFFFFFULL);
}

static inline uint8_t bh_irq_handle_domain_id(bh_irq_handle_t h) {
    return (uint8_t)((h >> 48) & 0xFFULL);
}

static inline uint8_t bh_irq_handle_type(bh_irq_handle_t h) {
    return (uint8_t)((h >> 56) & 0xFFULL);
}

// --- Dynamic Vector Allocator ---
typedef struct {
    uint32_t start;
    uint32_t count;
} bh_irq_vector_set_t;

kstatus_t bh_irq_vector_alloc(uint32_t count, uint32_t alignment, uint32_t flags, bh_irq_vector_set_t *out);
void bh_irq_vector_free(const bh_irq_vector_set_t *set);

// --- Core Lifecycle ---
void bh_irq_init_boot(void);
void bh_irq_init_cpu_local(uint32_t cpu_id);

kstatus_t bh_irq_register(uint32_t virq, bh_irq_handler_t handler, void* ctx, uint32_t flags, const char* name, void* dev_id, bh_irq_handle_t* out_handle);
kstatus_t bh_irq_unregister(bh_irq_handle_t handle, void* dev_id);
kstatus_t bh_irq_dispatch(uint32_t virq);
bool bh_irq_is_registered(uint32_t virq);
kstatus_t bh_irq_synchronize(bh_irq_handle_t handle, uint64_t deadline_ticks);

kstatus_t bh_irq_set_affinity(bh_irq_handle_t handle, bh_irq_affinity_t affinity);
kstatus_t bh_irq_get_affinity(bh_irq_handle_t handle, bh_irq_affinity_t* out_affinity);
uint32_t bh_irq_pick_target_cpu(uint32_t virq);

kstatus_t bh_irq_set_controller(uint32_t virq, const bh_irq_controller_ops_t* ops);

bh_irq_desc_t* bh_irq_get_descriptor(uint32_t virq);

// --- Deferred Work (Bottom-Half) ---
typedef struct bh_irq_deferred_item {
    uint32_t state; // FREE (0), READY (1), etc.
    bh_irq_deferred_cb_t callback;
    void *ctx;
    uint32_t source_virq;
    uint32_t source_generation;
} bh_irq_deferred_item_t;

kstatus_t bh_irq_defer_submit(bh_irq_deferred_cb_t callback, void *ctx, bh_irq_handle_t source);
void bh_irq_process_deferred(void);
void bh_irq_deferred_init_cpu_local(uint32_t cpu_id);

#endif // BHARAT_IRQ_H
