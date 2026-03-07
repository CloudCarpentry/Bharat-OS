#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/atomic.h"
#include "../../include/profile.h"
#include "../../include/spinlock.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_ORDER 11
#define MAX_NUMA_NODES 4
#define MAX_PHYS_PAGES 1048576
#define MAX_PAGES_PER_NODE (MAX_PHYS_PAGES / MAX_NUMA_NODES)
#define PMM_RECLAIM_BATCH 32U
#define PMM_LOW_WATERMARK_PAGES 128U

typedef struct {
    spinlock_t lock;
    list_head_t free_list[MAX_ORDER];
    size_t free_count[MAX_ORDER];
    phys_addr_t reclaim_pool[PMM_RECLAIM_BATCH];
    uint32_t reclaim_count;
} zone_t;

static page_t global_pages[MAX_PHYS_PAGES];
static zone_t numa_zones[MAX_NUMA_NODES];
static numa_node_t numa_nodes[MAX_NUMA_NODES];
static uint32_t active_numa_nodes;

phys_addr_t page_to_phys(page_t *page) {
    size_t global_index = (size_t)(page - global_pages);
    uint32_t node_id = (uint32_t)(global_index / MAX_PAGES_PER_NODE);
    size_t node_index = global_index % MAX_PAGES_PER_NODE;
    return numa_nodes[node_id].start_addr + (node_index * PAGE_SIZE);
}

page_t *phys_to_page(phys_addr_t phys) {
    for (uint32_t i = 0; i < active_numa_nodes; ++i) {
        numa_node_t *node = &numa_nodes[i];
        phys_addr_t end = node->start_addr + (node->total_pages * PAGE_SIZE);
        if (phys >= node->start_addr && phys < end) {
            size_t node_index = (size_t)((phys - node->start_addr) / PAGE_SIZE);
            return &global_pages[(i * MAX_PAGES_PER_NODE) + node_index];
        }
    }
    return NULL;
}

static void pmm_reclaim_one_node(uint32_t node_id) {
    zone_t *zone = &numa_zones[node_id];
    if (zone->reclaim_count == 0U) {
        return;
    }

    spin_lock(&zone->lock);
    while (zone->reclaim_count > 0U) {
        phys_addr_t page = zone->reclaim_pool[zone->reclaim_count - 1U];
        zone->reclaim_count--;
        spin_unlock(&zone->lock);
        mm_free_page(page);
        spin_lock(&zone->lock);
    }
    spin_unlock(&zone->lock);
}

static void* pmm_alloc_pages_order(int order, uint32_t numa_node) {
    zone_t *zone = &numa_zones[numa_node];

    spin_lock(&zone->lock);
    for (int level = order; level < MAX_ORDER; ++level) {
        if (!list_empty(&zone->free_list[level])) {
            page_t *page = list_entry(zone->free_list[level].next, page_t, list);
            list_del(&page->list);
            zone->free_count[level]--;

            while (level > order) {
                --level;
                page_t *buddy = page + (1 << level);
                buddy->order = level;
                buddy->ref_count = 0;
                list_add(&buddy->list, &zone->free_list[level]);
                zone->free_count[level]++;
            }

            page->order = order;
            page->ref_count = 1;
            spin_unlock(&zone->lock);
            return (void *)(uintptr_t)page_to_phys(page);
        }
    }
    spin_unlock(&zone->lock);

    pmm_reclaim_one_node(numa_node);

    spin_lock(&zone->lock);
    if (!list_empty(&zone->free_list[order])) {
        page_t *page = list_entry(zone->free_list[order].next, page_t, list);
        list_del(&page->list);
        zone->free_count[order]--;
        page->order = order;
        page->ref_count = 1;
        spin_unlock(&zone->lock);
        return (void *)(uintptr_t)page_to_phys(page);
    }
    spin_unlock(&zone->lock);

    return NULL;
}

int mm_pmm_init(void* memory_map, uint32_t map_size) {
    (void)memory_map;
    (void)map_size;

    active_numa_nodes = 2U;

    for (uint32_t i = 0; i < active_numa_nodes; ++i) {
        numa_nodes[i].node_id = i;
        numa_nodes[i].start_addr = (phys_addr_t)i * (MAX_PAGES_PER_NODE * PAGE_SIZE);
        numa_nodes[i].total_pages = MAX_PAGES_PER_NODE;
        numa_nodes[i].free_pages = 0;
        numa_nodes[i].allocator_metadata = &numa_zones[i];

        zone_t *zone = &numa_zones[i];
        spin_lock_init(&zone->lock);
        zone->reclaim_count = 0U;

        for (int order = 0; order < MAX_ORDER; ++order) {
            list_init(&zone->free_list[order]);
            zone->free_count[order] = 0;
        }

        for (size_t j = 0; j < MAX_PAGES_PER_NODE; ++j) {
            page_t *p = &global_pages[(i * MAX_PAGES_PER_NODE) + j];
            p->ref_count = 1;
            p->numa_node = (uint16_t)i;
            p->flags = 0;
            p->order = -1;
            list_init(&p->list);
        }
    }

    for (uint32_t i = 0; i < active_numa_nodes; ++i) {
        for (size_t j = 16; j < (MAX_PAGES_PER_NODE / 3U); ++j) {
            phys_addr_t phys = numa_nodes[i].start_addr + ((phys_addr_t)j * PAGE_SIZE);
            mm_free_page(phys);
        }
    }

    return 0;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    uint32_t home = preferred_numa_node;

    if (preferred_numa_node == NUMA_NODE_ANY || preferred_numa_node >= active_numa_nodes) {
        memory_node_id_t current = numa_get_current_node();
        home = (current < active_numa_nodes) ? current : 0U;
    }

    for (uint32_t attempt = 0; attempt < active_numa_nodes; ++attempt) {
        uint32_t node_id = (home + attempt) % active_numa_nodes;

        if (numa_nodes[node_id].free_pages < PMM_LOW_WATERMARK_PAGES) {
            pmm_reclaim_one_node(node_id);
        }

        void *addr = pmm_alloc_pages_order(0, node_id);
        if (addr) {
            atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages, 1U);
            return (phys_addr_t)(uintptr_t)addr;
        }
    }

    return 0;
}

void mm_free_page(phys_addr_t page_addr) {
    page_t *page = phys_to_page(page_addr);
    if (!page) {
        return;
    }

    uint16_t old_ref = atomic16_fetch_and_sub(&page->ref_count, 1);
    if (old_ref != 1U) {
        return;
    }

    uint32_t node_id = page->numa_node;
    zone_t *zone = &numa_zones[node_id];
    size_t node_index = (size_t)((page_addr - numa_nodes[node_id].start_addr) / PAGE_SIZE);

    int order = page->order;
    if (order < 0) {
        order = 0;
    }

    spin_lock(&zone->lock);
    while (order < (MAX_ORDER - 1)) {
        size_t buddy_index = node_index ^ (1ULL << order);
        size_t global_idx = (node_id * MAX_PAGES_PER_NODE) + buddy_index;
        page_t *buddy = &global_pages[global_idx];

        if (buddy->ref_count > 0U || buddy->order != order) {
            break;
        }

        list_del(&buddy->list);
        zone->free_count[order]--;

        if (buddy_index < node_index) {
            page = buddy;
            node_index = buddy_index;
        }

        ++order;
    }

    page->order = order;
    page->ref_count = 0;
    list_add(&page->list, &zone->free_list[order]);
    zone->free_count[order]++;
    spin_unlock(&zone->lock);

    atomic64_fetch_and_add_ptr(&numa_nodes[node_id].free_pages, 1U);
}

#ifndef Profile_RTOS
void mm_inc_page_ref(phys_addr_t page_addr) {
    page_t *page = phys_to_page(page_addr);
    if (page && page->ref_count > 0U) {
        atomic16_fetch_and_add(&page->ref_count, 1U);
    }
}
#else
void mm_inc_page_ref(phys_addr_t page_addr) {
    (void)page_addr;
}
#endif
