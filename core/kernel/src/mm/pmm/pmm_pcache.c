#include "mm/pmm_pcache.h"
#include "hal/hal.h"
#include "spinlock.h"

pmm_core_state_t g_pmm_cores[BHARAT_MAX_CPUS];
static uint32_t g_pmm_active_cpu_count = BHARAT_MAX_CPUS;

bool pmm_core_id_valid(uint32_t core_id) {
    return core_id < g_pmm_active_cpu_count && core_id < BHARAT_MAX_CPUS;
}

bool pmm_numa_node_valid(uint32_t node_id) {
    extern uint32_t active_numa_nodes;
    return node_id < active_numa_nodes && node_id < 4;
}

bool pmm_page_can_enter_pcache(struct page *page) {
    if (!page) return false;
    // Pinned pages cannot enter pcache
    if (__atomic_load_n(&page->pin_count, __ATOMIC_ACQUIRE) > 0) return false;
    // Only pages with 0 or 1 reference (about to be 0) can enter pcache
    // In mm_free_page, we check if ref_count is 1 or 0.
    return true;
}

bool pmm_page_owned_by_core(struct page *page, uint32_t core_id) {
    if (!page) return false;
    return page->owner_core_id == core_id;
}

bool pmm_remote_inbox_has_space(uint32_t core_id) {
    if (!pmm_core_id_valid(core_id)) return false;
    pmm_remote_inbox_t *inbox = &g_pmm_cores[core_id].inbox;
    uint32_t next_head = (inbox->head + 1) % PMM_INBOX_SIZE;
    return next_head != inbox->tail;
}

void pmm_pcache_init_all(void) {
    // In a real system, we might discover this from boot info
    // For now, default to BHARAT_MAX_CPUS or a safe detected limit.
    g_pmm_active_cpu_count = BHARAT_MAX_CPUS;

    for (int i = 0; i < BHARAT_MAX_CPUS; i++) {
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
    if (pmm_core_id_valid(core_id)) {
        g_pmm_cores[core_id].active = true;
    }
}

void pmm_drain_remote_frees(uint32_t core_id) {
    if (!pmm_core_id_valid(core_id)) return;

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

kstatus_t pmm_pcache_remote_free_enqueue(uint32_t core_id, phys_addr_t page_addr) {
    if (!pmm_core_id_valid(core_id)) return K_ERR_INVALID_ARG;

    pmm_core_state_t *core_state = &g_pmm_cores[core_id];
    if (!core_state->active) return K_ERR_BAD_STATE;

    pmm_remote_inbox_t *inbox = &core_state->inbox;

    spin_lock(&inbox->lock);

    uint32_t next_head = (inbox->head + 1) % PMM_INBOX_SIZE;
    if (next_head == inbox->tail) {
        inbox->enqueue_failures++;
        spin_unlock(&inbox->lock);
        return K_ERR_NO_MEMORY; // Inbox full
    }

    inbox->pages[inbox->head] = page_addr;
    inbox->head = next_head;
    inbox->enqueue_count++;

    spin_unlock(&inbox->lock);
    return K_OK;
}
