#include "irq/bh_irq.h"
#include "hal/hal.h"
#include "bharat/kernel/ds/bh_mpsc_queue.h"

#define BH_IRQ_DEFERRED_QUEUE_CAPACITY 128U

static bh_mpsc_queue_t g_deferred_queues[MAX_CPUS];
static bh_mpsc_slot_t g_deferred_slots[MAX_CPUS][BH_IRQ_DEFERRED_QUEUE_CAPACITY];
static bh_irq_deferred_item_t g_deferred_pools[MAX_CPUS][BH_IRQ_DEFERRED_QUEUE_CAPACITY];

void bh_irq_deferred_init_cpu_local(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPUS) return;

    bh_mpsc_queue_init(&g_deferred_queues[cpu_id], g_deferred_slots[cpu_id], BH_IRQ_DEFERRED_QUEUE_CAPACITY);

    for (uint32_t i = 0; i < BH_IRQ_DEFERRED_QUEUE_CAPACITY; i++) {
        __atomic_store_n(&g_deferred_pools[cpu_id][i].state, 0, __ATOMIC_SEQ_CST);
        g_deferred_pools[cpu_id][i].callback = NULL;
        g_deferred_pools[cpu_id][i].ctx = NULL;
        g_deferred_pools[cpu_id][i].source_virq = 0;
        g_deferred_pools[cpu_id][i].source_generation = 0;
    }
}

kstatus_t bh_irq_defer_submit(bh_irq_deferred_cb_t callback, void *ctx, bh_irq_handle_t source) {
    if (!callback) return K_ERR_INVALID_ARG;

    uint32_t cpu_id = hal_cpu_get_id();
    if (cpu_id >= MAX_CPUS) return K_ERR_INVALID_ARG;

    // Scan pool to find a free slot
    int free_idx = -1;
    for (uint32_t i = 0; i < BH_IRQ_DEFERRED_QUEUE_CAPACITY; i++) {
        uint32_t expected = 0;
        if (__atomic_load_n(&g_deferred_pools[cpu_id][i].state, __ATOMIC_SEQ_CST) == 0) {
            if (__atomic_compare_exchange_n(&g_deferred_pools[cpu_id][i].state, &expected, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                free_idx = (int)i;
                break;
            }
        }
    }

    if (free_idx == -1) {
        // Increment dropped deferred count on descriptor if valid
        uint32_t virq = bh_irq_handle_slot(source);
        bh_irq_desc_t *desc = bh_irq_get_descriptor(virq);
        if (desc) {
            desc->dropped_deferred_count++;
        }
        return K_ERR_AGAIN;
    }

    g_deferred_pools[cpu_id][free_idx].callback = callback;
    g_deferred_pools[cpu_id][free_idx].ctx = ctx;
    g_deferred_pools[cpu_id][free_idx].source_virq = bh_irq_handle_slot(source);
    g_deferred_pools[cpu_id][free_idx].source_generation = bh_irq_handle_generation(source);

    kstatus_t status = bh_mpsc_queue_push(&g_deferred_queues[cpu_id], &g_deferred_pools[cpu_id][free_idx]);
    if (status != K_OK) {
        __atomic_store_n(&g_deferred_pools[cpu_id][free_idx].state, 0, __ATOMIC_SEQ_CST);
        uint32_t virq = bh_irq_handle_slot(source);
        bh_irq_desc_t *desc = bh_irq_get_descriptor(virq);
        if (desc) {
            desc->dropped_deferred_count++;
        }
        return K_ERR_AGAIN;
    }

    // Increment successfully deferred count on descriptor
    uint32_t virq = bh_irq_handle_slot(source);
    bh_irq_desc_t *desc = bh_irq_get_descriptor(virq);
    if (desc) {
        desc->deferred_count++;
    }

    return K_OK;
}

void bh_irq_process_deferred(void) {
    uint32_t cpu_id = hal_cpu_get_id();
    if (cpu_id >= MAX_CPUS) return;

    uint32_t count = 0;
    void *val = NULL;

    // Process a bounded batch of up to 64 items to keep latency low
    while (count < 64 && bh_mpsc_queue_pop(&g_deferred_queues[cpu_id], &val) == K_OK) {
        bh_irq_deferred_item_t *item = (bh_irq_deferred_item_t *)val;
        if (item) {
            if (item->callback) {
                item->callback(item->ctx);
            }
            __atomic_store_n(&item->state, 0, __ATOMIC_SEQ_CST);
        }
        count++;
    }
}
