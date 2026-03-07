#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/atomic.h"
#include "../../include/profile.h"
#include "../../include/spinlock.h"
#include "early_alloc.h"

// Multiboot only for x86_64
#if defined(__x86_64__)
#include "../boot/x86_64/multiboot2.h"
#endif

#include <stddef.h>
#include <stdint.h>

#define MAX_ORDER 12 // allows order 11 -> 2048 pages -> 8MB, and order 9 -> 512 pages -> 2MB
#define MAX_NUMA_NODES 4
#define PMM_RECLAIM_BATCH 32U
#define PMM_LOW_WATERMARK_PAGES 128U

typedef struct {
    spinlock_t lock;
    list_head_t free_list[MAX_ORDER];
    size_t free_count[MAX_ORDER];
    phys_addr_t reclaim_pool[PMM_RECLAIM_BATCH];
    uint32_t reclaim_count;
} zone_t;

static page_t* global_pages_ptrs[MAX_NUMA_NODES];
static zone_t numa_zones[MAX_NUMA_NODES];
static numa_node_t numa_nodes[MAX_NUMA_NODES];
static uint32_t active_numa_nodes;

phys_addr_t page_to_phys(page_t *page) {
    uint32_t node_id = page->numa_node;
    size_t node_index = (size_t)(page - global_pages_ptrs[node_id]);
    return numa_nodes[node_id].start_addr + (node_index * PAGE_SIZE);
}

page_t *phys_to_page(phys_addr_t phys) {
    for (uint32_t i = 0; i < active_numa_nodes; ++i) {
        numa_node_t *node = &numa_nodes[i];
        phys_addr_t end = node->start_addr + (node->total_pages * PAGE_SIZE);
        if (phys >= node->start_addr && phys < end) {
            size_t node_index = (size_t)((phys - node->start_addr) / PAGE_SIZE);
            return &global_pages_ptrs[i][node_index];
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
                size_t offset = (1ULL << level);
                page_t *buddy = page + offset;
                buddy->order = level;
                buddy->ref_count = 0;
                buddy->flags = 0;
                list_add(&buddy->list, &zone->free_list[level]);
                zone->free_count[level]++;
            }

            page->order = order;
            page->ref_count = 1;
            page->flags = PAGE_FLAG_KERNEL; // By default give kernel pages
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
        page->flags = PAGE_FLAG_KERNEL;
        spin_unlock(&zone->lock);
        return (void *)(uintptr_t)page_to_phys(page);
    }
    spin_unlock(&zone->lock);

    return NULL;
}

phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) {
    uint32_t home = preferred_numa_node;

    if (preferred_numa_node == NUMA_NODE_ANY || preferred_numa_node >= active_numa_nodes) {
        memory_node_id_t current = numa_get_current_node();
        home = (current < active_numa_nodes) ? current : 0U;
    }

    for (uint32_t attempt = 0; attempt < active_numa_nodes; ++attempt) {
        uint32_t node_id = (home + attempt) % active_numa_nodes;

        // If not enough free pages and nothing to reclaim, continue
        if (numa_nodes[node_id].free_pages < (1ULL << order)) {
            pmm_reclaim_one_node(node_id);
            if (numa_nodes[node_id].free_pages < (1ULL << order)) continue;
        }

        void *addr = pmm_alloc_pages_order(order, node_id);
        if (addr) {
            atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages, (1ULL << order));
            page_t* page = phys_to_page((phys_addr_t)(uintptr_t)addr);
            if (page) {
                page->flags = flags;
            }
            return (phys_addr_t)(uintptr_t)addr;
        }
    }

    return 0;
}

static void mark_page_free(phys_addr_t phys) {
    page_t *p = phys_to_page(phys);
    if (!p) {
        return;
    }
    p->ref_count = 1;
    p->order = 0; // Ensure initial order is 0 when freed into the buddy system!
    mm_free_page(phys);
}

int mm_pmm_init(void* memory_map, uint32_t map_size) {
    (void)map_size;
    if (!memory_map) return -1;

    phys_addr_t highest_addr = 0;
    phys_addr_t lowest_addr = (phys_addr_t)-1;

#if defined(__x86_64__)
    multiboot_information_t* mb_info = (multiboot_information_t*)memory_map;
    uint32_t total_size = mb_info->total_size;

    // First pass: find available memory bounds
    uint8_t* tag_ptr = (uint8_t*)mb_info + 8; // skip information header
    while (tag_ptr < (uint8_t*)mb_info + total_size) {
        multiboot_tag_t* tag = (multiboot_tag_t*)tag_ptr;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            multiboot_tag_mmap_t* mmap_tag = (multiboot_tag_mmap_t*)tag;
            uint32_t entry_size = mmap_tag->entry_size;
            if (entry_size < sizeof(multiboot_mmap_entry_t)) {
                entry_size = sizeof(multiboot_mmap_entry_t);
            }

            // Fix: the size field in multiboot_tag_mmap_t includes the tag header itself
            uint32_t num_entries = 0;
            if (mmap_tag->size > 16) { // 16 is size of multiboot_tag_mmap_t base struct
                num_entries = (mmap_tag->size - 16) / entry_size;
            }

            for (uint32_t i = 0; i < num_entries; i++) {
                multiboot_mmap_entry_t* entry = (multiboot_mmap_entry_t*)((uint8_t*)mmap_tag->entries + (i * entry_size));
                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                    if (entry->addr < lowest_addr) lowest_addr = entry->addr;
                    if (entry->addr + entry->len > highest_addr) highest_addr = entry->addr + entry->len;
                }
            }
        }

        // Align to 8 bytes per multiboot2 spec
        uint32_t tag_size = tag->size;
        if (tag_size < 8) tag_size = 8;
        uint32_t aligned_size = (tag_size + 7) & ~7;
        if (aligned_size == 0) break; // Prevent infinite loop on bad struct
        tag_ptr += aligned_size;
    }
#endif

    if (highest_addr == 0) return -1; // No memory found

    early_alloc_init(0); // Uses _end

    // For now, assign everything to NUMA node 0
    active_numa_nodes = 1U;

    // Ensure start is page aligned
    lowest_addr = lowest_addr & ~(PAGE_SIZE - 1);

    // Calculate total pages
    uint64_t total_pages = (highest_addr - lowest_addr) / PAGE_SIZE;

    numa_nodes[0].node_id = 0;
    numa_nodes[0].start_addr = lowest_addr;
    numa_nodes[0].total_pages = total_pages;
    numa_nodes[0].free_pages = 0;
    numa_nodes[0].allocator_metadata = &numa_zones[0];

    // Allocate page structures for Node 0
    size_t page_array_size = total_pages * sizeof(page_t);
    global_pages_ptrs[0] = (page_t*)early_alloc(page_array_size, PAGE_SIZE);

    if (!global_pages_ptrs[0]) {
        return -1;
    }

    zone_t *zone = &numa_zones[0];
    spin_lock_init(&zone->lock);
    zone->reclaim_count = 0U;

    for (int order = 0; order < MAX_ORDER; ++order) {
        list_init(&zone->free_list[order]);
        zone->free_count[order] = 0;
    }

    // Initialize all pages as reserved/allocated (ref_count = 1)
    for (size_t j = 0; j < total_pages; ++j) {
        page_t *p = &global_pages_ptrs[0][j];
        p->ref_count = 1;
        p->numa_node = 0;
        p->flags = PAGE_FLAG_RESERVED;
        p->order = -1;
        list_init(&p->list);
    }

    // Determine the safe start address for available memory.
    // It must be strictly after the kernel _end and the early allocator bump ptr.
    phys_addr_t early_mem_end = early_alloc_get_current_ptr();
    early_mem_end = (early_mem_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // Page align up

#if defined(__x86_64__)
    // Second pass: free available memory pages to the buddy allocator
    tag_ptr = (uint8_t*)mb_info + 8;
    while (tag_ptr < (uint8_t*)mb_info + total_size) {
        multiboot_tag_t* tag = (multiboot_tag_t*)tag_ptr;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            multiboot_tag_mmap_t* mmap_tag = (multiboot_tag_mmap_t*)tag;
            uint32_t entry_size = mmap_tag->entry_size;
            if (entry_size < sizeof(multiboot_mmap_entry_t)) {
                entry_size = sizeof(multiboot_mmap_entry_t);
            }
            uint32_t num_entries = 0;
            if (mmap_tag->size > 16) {
                num_entries = (mmap_tag->size - 16) / entry_size;
            }

            for (uint32_t i = 0; i < num_entries; i++) {
                multiboot_mmap_entry_t* entry = (multiboot_mmap_entry_t*)((uint8_t*)mmap_tag->entries + (i * entry_size));
                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                    phys_addr_t region_start = entry->addr;
                    phys_addr_t region_end = entry->addr + entry->len;

                    // Skip the physical region where kernel and early boot data live
                    // Multiboot loaded us at 1MB, early allocator works past _end.
                    // For safety, assume 0 to early_mem_end is completely off limits.
                    if (region_start < early_mem_end) {
                        region_start = early_mem_end;
                    }

                    // Align start up, end down
                    region_start = (region_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
                    region_end = region_end & ~(PAGE_SIZE - 1);

                    if (region_start < region_end) {
                        for (phys_addr_t p = region_start; p < region_end; p += PAGE_SIZE) {
                            mark_page_free(p);
                        }
                    }
                }
            }
        }

        uint32_t tag_size = tag->size;
        if (tag_size < 8) tag_size = 8;
        uint32_t aligned_size = (tag_size + 7) & ~7;
        if (aligned_size == 0) break;
        tag_ptr += aligned_size;
    }
#endif

    return 0;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    return mm_alloc_pages_order(0, preferred_numa_node, PAGE_FLAG_KERNEL);
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
        if (buddy_index >= numa_nodes[node_id].total_pages) {
            break;
        }

        page_t *buddy = &global_pages_ptrs[node_id][buddy_index];

        if (buddy->ref_count > 0U || buddy->order != order) {
            break;
        }

        // Check if buddy is in free list
        int found = 0;
        list_head_t *pos;
        for (pos = zone->free_list[order].next; pos != &zone->free_list[order]; pos = pos->next) {
            if (pos == &buddy->list) {
                found = 1;
                break;
            }
        }
        if (!found) {
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
    page->flags = 0;
    list_add(&page->list, &zone->free_list[order]);
    zone->free_count[order]++;
    spin_unlock(&zone->lock);

    atomic64_fetch_and_add_ptr(&numa_nodes[node_id].free_pages, (1ULL << order));
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
