#include "mm/pmm_pcache.h"
#include "hal/hal.h"
#include "spinlock.h"

// Assuming MAX_CORES 256 for now, can be sized dynamically if needed.
pmm_core_state_t g_pmm_cores[256];

void pmm_pcache_init_all(void) {
    for (int i = 0; i < 256; i++) {
        g_pmm_cores[i].active = false;
        spin_lock_init(&g_pmm_cores[i].inbox.lock);
        g_pmm_cores[i].inbox.head = 0;
        g_pmm_cores[i].inbox.tail = 0;
        g_pmm_cores[i].inbox.enqueue_count = 0;
        g_pmm_cores[i].inbox.enqueue_failures = 0;
        g_pmm_cores[i].inbox.drain_batches = 0;
        g_pmm_cores[i].inbox.drain_pages = 0;

        for (int j = 0; j < 4; j++) {
            g_pmm_cores[i].node_caches[j].count = 0;
            g_pmm_cores[i].node_caches[j].alloc_hits = 0;
            g_pmm_cores[i].node_caches[j].alloc_misses = 0;
            g_pmm_cores[i].node_caches[j].refill_count = 0;
            g_pmm_cores[i].node_caches[j].refill_pages = 0;
            g_pmm_cores[i].node_caches[j].local_frees = 0;
            g_pmm_cores[i].node_caches[j].drain_to_zone_count = 0;
            g_pmm_cores[i].node_caches[j].direct_zone_allocs = 0;
            g_pmm_cores[i].node_caches[j].direct_zone_frees = 0;
        }
    }
}

void pmm_core_local_init(uint32_t core_id) {
    if (core_id < 256) {
        g_pmm_cores[core_id].active = true;
    }
}

void pmm_drain_remote_frees(uint32_t core_id) {
    if (core_id >= 256) return;

    pmm_core_state_t *core_state = &g_pmm_cores[core_id];
    if (!core_state->active) return;

    pmm_remote_inbox_t *inbox = &core_state->inbox;
    uint32_t drained = 0;

    spin_lock(&inbox->lock);

    while (inbox->head != inbox->tail && drained < PMM_REMOTE_BATCH) {
        phys_addr_t page_addr = inbox->pages[inbox->tail];
        inbox->tail = (inbox->tail + 1) % PMM_INBOX_SIZE;
        drained++;

        spin_unlock(&inbox->lock);

        // This acts like a local free since we are the owner core
        mm_free_page(page_addr);

        spin_lock(&inbox->lock);
    }

    if (drained > 0) {
        inbox->drain_batches++;
        inbox->drain_pages += drained;
    }

    spin_unlock(&inbox->lock);
}
