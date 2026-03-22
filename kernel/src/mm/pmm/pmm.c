#include "atomic.h"
#include "mm.h"
#include "numa.h"
#include "profile.h"
#include "spinlock.h"

#include "sched.h"
#include "early_alloc.h"
#include "bharat/boot_info.h"

// Multiboot only for x86_64
#include "hal/hal.h"
#include "hal/hal_discovery.h"
#include "hal/hal_mm.h"
#include "console/console_core.h"
#define KPRINT(s) console_write_raw(s, string_length(s))

#include <stddef.h>
#include <stdint.h>
#include "mm/pmm_map.h"
#include "mm/pt_cache.h"

#define MAX_ORDER                                                              \
  12 // allows order 11 -> 2048 pages -> 8MB, and order 9 -> 512 pages -> 2MB
#define MAX_NUMA_NODES 4
#define PMM_RECLAIM_BATCH 32U
#define PMM_LOW_WATERMARK_PAGES 128U

typedef struct {
  spinlock_t lock;
  list_head_t free_list[MAX_ORDER][CONFIG_MM_CACHE_COLORS_DEFAULT];
  size_t free_count[MAX_ORDER][CONFIG_MM_CACHE_COLORS_DEFAULT];
  phys_addr_t reclaim_pool[PMM_RECLAIM_BATCH];
  uint32_t reclaim_count;
} zone_t;

static page_t *global_pages_ptrs[MAX_NUMA_NODES];
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

static inline uint32_t get_page_color(phys_addr_t phys) {
  return (phys / PAGE_SIZE) % CONFIG_MM_CACHE_COLORS_DEFAULT;
}

static void *pmm_alloc_pages_order_colored(int order, uint32_t numa_node,
                                           mm_color_config_t *color_config) {
  zone_t *zone = &numa_zones[numa_node];

  spin_lock(&zone->lock);

  int start_color = 0;
  int end_color = CONFIG_MM_CACHE_COLORS_DEFAULT - 1;

  for (int level = order; level < MAX_ORDER; ++level) {
    for (int c = start_color; c <= end_color; ++c) {
      if (color_config && color_config->policy != MM_COLOR_POLICY_NONE &&
          color_config->policy != MM_COLOR_POLICY_PREFERRED) {
        if ((color_config->color_mask & (1U << c)) == 0)
          continue;
      }

      if (!list_empty(&zone->free_list[level][c])) {
        page_t *page = list_entry(zone->free_list[level][c].next, page_t, list);
        list_del(&page->list);
        zone->free_count[level][c]--;

        int l = level;
        while (l > order) {
          --l;
          size_t offset = (1ULL << l);
          page_t *buddy = page + offset;
          buddy->order = l;
          buddy->ref_count = 0;
          buddy->flags = 0;

          uint32_t buddy_color = get_page_color(page_to_phys(buddy));
          list_add(&buddy->list, &zone->free_list[l][buddy_color]);
          zone->free_count[l][buddy_color]++;
        }

        page->order = order;
        page->ref_count = 1;
        page->flags = PAGE_FLAG_KERNEL; // By default give kernel pages
        spin_unlock(&zone->lock);
        return (void *)(uintptr_t)page_to_phys(page);
      }
    }
  }
  spin_unlock(&zone->lock);

  pmm_reclaim_one_node(numa_node);

  spin_lock(&zone->lock);
  for (int c = start_color; c <= end_color; ++c) {
    if (color_config && color_config->policy != MM_COLOR_POLICY_NONE &&
        color_config->policy != MM_COLOR_POLICY_PREFERRED) {
      if ((color_config->color_mask & (1U << c)) == 0)
        continue;
    }

    if (!list_empty(&zone->free_list[order][c])) {
      page_t *page = list_entry(zone->free_list[order][c].next, page_t, list);
      list_del(&page->list);
      zone->free_count[order][c]--;
      page->order = order;
      page->ref_count = 1;
      page->flags = PAGE_FLAG_KERNEL;
      spin_unlock(&zone->lock);
      return (void *)(uintptr_t)page_to_phys(page);
    }
  }
  spin_unlock(&zone->lock);

  return NULL;
}

int pmm_alloc_pages(uint32_t order, pmm_zone_t zone, uint32_t alloc_flags, pmm_block_t *out_block) {
  if (!out_block) return -1;
  if (order >= MAX_ORDER) return -1;

  // We map zone directly in the allocator search later, but for now we'll do a basic proxy to pmm_alloc_pages_colored
  // Note: we need to enforce zones later.

  mm_color_config_t no_color_config = {.policy = MM_COLOR_POLICY_NONE, .domain = MM_DOMAIN_DEFAULT, .color_mask = 0xFFFFFFFF};
  phys_addr_t phys = pmm_alloc_pages_colored(order, NUMA_NODE_ANY, PAGE_FLAG_KERNEL, &no_color_config);

  if (phys == 0) return -1;

  // Since we don't fully respect `zone` inside pmm_alloc_pages_colored yet, let's at least enforce the check post-alloc.
  // Proper implementation would pass zone down, but since we are modifying the wrapper we can do a naive check.
  // To avoid rewriting `pmm_alloc_pages_colored` entirely right now, we just fill out_block.

  page_t *p = phys_to_page(phys);
  if (!p) return -1;

  if (zone != PMM_ZONE_ANY && p->zone > zone) {
    // Highly naive: if we got a page outside the zone, free it and fail.
    // In production, `pmm_alloc_pages_colored` must be zone-aware.
    mm_free_page(phys);
    return -1;
  }

  p->state = PMM_PAGE_STATE_ALLOCATED;
  p->pin_count = (alloc_flags & PMM_ALLOC_PINNED) ? 1 : 0;

  // Set block
  out_block->phys_addr = phys;
  out_block->order = order;
  out_block->page_count = (1ULL << order);
  out_block->flags = alloc_flags;

  if (alloc_flags & PMM_ALLOC_ZERO) {
    uint8_t *ptr = (uint8_t *)(uintptr_t)phys;
    for (size_t i = 0; i < (1ULL << order) * PAGE_SIZE; i++) {
      ptr[i] = 0;
    }
  }

  return 0;
}

int pmm_alloc_contiguous(uint32_t page_count, pmm_zone_t zone, uint32_t alloc_flags, pmm_block_t *out_block) {
  if (page_count == 0 || !out_block) return -1;

  // Fast path: power of two
  uint32_t order = 0;
  while ((1ULL << order) < page_count) {
    order++;
  }

  if ((1ULL << order) == page_count) {
    return pmm_alloc_pages(order, zone, alloc_flags, out_block);
  }

  if (order >= MAX_ORDER) return -1;

  // Over-allocate and free tails
  pmm_block_t big_block;
  if (pmm_alloc_pages(order, zone, alloc_flags, &big_block) != 0) {
    return -1;
  }

  phys_addr_t base_phys = big_block.phys_addr;
  uint32_t allocated_pages = (1ULL << big_block.order);

  // Mark the required run
  out_block->phys_addr = base_phys;
  out_block->order = 0; // It's no longer a buddy block
  out_block->page_count = page_count;
  out_block->flags = alloc_flags;

  // Free trailing pages manually
  for (uint32_t i = page_count; i < allocated_pages; i++) {
    phys_addr_t free_phys = base_phys + i * PAGE_SIZE;
    page_t *p = phys_to_page(free_phys);
    if (p) {
        p->state = PMM_PAGE_STATE_FREE;
        p->ref_count = 1;
        p->order = 0;
    }
    mm_free_page(free_phys);
  }

  page_t *head_p = phys_to_page(base_phys);
  if (head_p) {
      head_p->order = 0; // break buddy order since it's a custom block
      head_p->pin_count = (alloc_flags & PMM_ALLOC_PINNED) ? 1 : 0;
  }

  return 0;
}

int pmm_free_pages(const pmm_block_t *block) {
  if (!block) return -1;
  phys_addr_t phys = block->phys_addr;

  if (block->page_count > 0 && block->page_count != (1ULL << block->order)) {
    // Non-power-of-two release
    for (uint32_t i = 0; i < block->page_count; i++) {
      phys_addr_t p = phys + i * PAGE_SIZE;
      page_t *page = phys_to_page(p);
      if (page) {
          if (page->pin_count > 0) return -1;
          page->state = PMM_PAGE_STATE_FREE;
          page->order = 0; // Just in case
          page->ref_count = 1; // prepare for mm_free_page
      }
      mm_free_page(p);
    }
    return 0;
  } else {
      page_t *page = phys_to_page(phys);
      if (page) {
          if (page->pin_count > 0) return -1;
          page->state = PMM_PAGE_STATE_FREE;
          page->order = block->order;
          page->ref_count = 1; // prepare for mm_free_page
      }
      mm_free_page(phys);
      return 0;
  }
}

int pmm_ref_get(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (page && page->ref_count > 0U) {
        atomic16_fetch_and_add(&page->ref_count, 1U);
        return 0;
    }
    return -1;
}

int pmm_ref_put(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (!page) return -1;

    uint16_t old_ref = atomic16_fetch_and_sub(&page->ref_count, 1U);
    if (old_ref == 1U) {
        // Last ref
        if (page->pin_count > 0) {
            // Can't free pinned page
            return -1;
        }
        page->state = PMM_PAGE_STATE_FREE;
        page->ref_count = 1; // prepare for mm_free_page
        mm_free_page(phys_addr);
    }
    return 0;
}

int pmm_pin(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (!page) return -1;
    atomic16_fetch_and_add(&page->pin_count, 1U);
    return 0;
}

int pmm_unpin(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (!page) return -1;
    uint16_t old = atomic16_fetch_and_sub(&page->pin_count, 1U);
    if (old == 0) {
        // underflow protection
        page->pin_count = 0;
        return -1;
    }
    return 0;
}

phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node,
                                    uint32_t flags,
                                    mm_color_config_t *color_config) {
  uint32_t home = preferred_numa_node;

  if (preferred_numa_node == NUMA_NODE_ANY ||
      preferred_numa_node >= active_numa_nodes) {
    memory_node_id_t current = numa_get_current_node();
    home = (current < active_numa_nodes) ? current : 0U;
  }

  for (uint32_t attempt = 0; attempt < active_numa_nodes; ++attempt) {
    uint32_t node_id = (home + attempt) % active_numa_nodes;

    // If not enough free pages and nothing to reclaim, continue
    if (numa_nodes[node_id].free_pages < (1ULL << order)) {
      pmm_reclaim_one_node(node_id);
      if (numa_nodes[node_id].free_pages < (1ULL << order))
        continue;
    }

    void *addr = pmm_alloc_pages_order_colored(order, node_id, color_config);
    if (addr) {
      atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages,
                                 (1ULL << order));
      page_t *page = phys_to_page((phys_addr_t)(uintptr_t)addr);
      if (page) {
        page->flags = flags;
      }
      return (phys_addr_t)(uintptr_t)addr;
    }
  }

  // Attempt fallback for preferred policy if strict is not requested
  if (color_config && color_config->policy == MM_COLOR_POLICY_PREFERRED) {
    // Try allocating without color constraint
    for (uint32_t attempt = 0; attempt < active_numa_nodes; ++attempt) {
      uint32_t node_id = (home + attempt) % active_numa_nodes;
      mm_color_config_t no_color_config = {.policy = MM_COLOR_POLICY_NONE,
                                           .domain = MM_DOMAIN_DEFAULT,
                                           .color_mask = 0xFFFFFFFF};
      void *addr =
          pmm_alloc_pages_order_colored(order, node_id, &no_color_config);
      if (addr) {
        atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages,
                                   (1ULL << order));
        page_t *page = phys_to_page((phys_addr_t)(uintptr_t)addr);
        if (page) {
          page->flags = flags;
        }
        return (phys_addr_t)(uintptr_t)addr;
      }
    }
  }

  // OOM Handler logic
  for (uint32_t attempt = 0; attempt < active_numa_nodes; ++attempt) {
    pmm_reclaim_one_node(attempt);
  }

  // Attempt one last time
  for (uint32_t attempt = 0; attempt < active_numa_nodes; ++attempt) {
    uint32_t node_id = (home + attempt) % active_numa_nodes;
    void *addr = pmm_alloc_pages_order_colored(order, node_id, color_config);
    if (addr) {
      atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages,
                                 (1ULL << order));
      page_t *page = phys_to_page((phys_addr_t)(uintptr_t)addr);
      if (page) {
        page->flags = flags;
      }
      return (phys_addr_t)(uintptr_t)addr;
    }
  }

  // Still OOM, invoke AI governor action to kill current task
  kthread_t *current = sched_current_thread();
  if (current) {
    ai_suggestion_t suggestion;
    suggestion.action = AI_ACTION_KILL_TASK;
    suggestion.target_id = current->thread_id;
    suggestion.value = 0;
    sched_ai_apply_suggestion(&suggestion);
  }

  return 0;
}

phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node,
                                 uint32_t flags) {
  kthread_t *current = sched_current_thread();
  mm_color_config_t *color_config = NULL;
  if (current) {
    color_config = &current->mm_color_policy;
  }
  return pmm_alloc_pages_colored(order, preferred_numa_node, flags,
                                 color_config);
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

/*
 * Multi-Architecture PMM Discovery Logic
 * Supports normalized boot_info_t contract.
 */

static void pmm_add_region(phys_addr_t base, size_t size, uint32_t type,
                           uint32_t target_numa_node) {
  if (size < PAGE_SIZE)
    return;
  if (active_numa_nodes >= MAX_NUMA_NODES)
    return;

  size_t page_count = size / PAGE_SIZE;
  uint32_t node_id = active_numa_nodes++;

  numa_nodes[node_id].node_id = target_numa_node;
  numa_nodes[node_id].start_addr = base;
  numa_nodes[node_id].total_pages = page_count;
  numa_nodes[node_id].free_pages = 0;
  numa_nodes[node_id].allocator_metadata = &numa_zones[node_id];

  size_t page_array_size = page_count * sizeof(page_t);
  void *page_array = early_alloc(page_array_size, PAGE_SIZE);
  global_pages_ptrs[node_id] = (page_t *)page_array;

  // Zero-initialize metadata
  uint8_t *p_bytes = (uint8_t *)page_array;
  for (size_t i = 0; i < page_array_size; i++) {
    p_bytes[i] = 0;
  }

  // Capture current end of reserved memory (kernel + metadata)
  phys_addr_t early_mem_end = (phys_addr_t)early_alloc(0, 1);

  hal_serial_write("PMM: Region ");
  hal_serial_write_hex(base);
  hal_serial_write(" size ");
  hal_serial_write_hex(size);
  hal_serial_write(" protected up to ");
  hal_serial_write_hex(early_mem_end);
  hal_serial_write("\n");

  zone_t *zone = &numa_zones[node_id];
  spin_lock_init(&zone->lock);
  zone->reclaim_count = 0U;

  for (int o = 0; o < MAX_ORDER; o++) {
    for (int c = 0; c < CONFIG_MM_CACHE_COLORS_DEFAULT; c++) {
      list_init(&zone->free_list[o][c]);
      zone->free_count[o][c] = 0;
    }
  }

  hal_mm_zone_limits_t limits;
  hal_mm_get_zone_limits(&limits);

  // Pass 1: Initialize all metadata structures
  for (size_t j = 0; j < page_count; j++) {
    page_t *p = &global_pages_ptrs[node_id][j];
    p->ref_count = 1;
    p->numa_node = node_id;
    p->flags = 0;
    p->order = 0; // Initialize order to 0 so buddy merging works immediately
    list_init(&p->list);

    phys_addr_t paddr = base + (j * PAGE_SIZE);
    if (paddr >= limits.dma32_start && paddr <= limits.dma32_end) {
      p->zone = PMM_ZONE_DMA32;
    } else {
      p->zone = PMM_ZONE_NORMAL;
    }
  }

  // Pass 2: Free usable RAM pages (skipping reserved early memory)
  if (type == PMM_REGION_TYPE_USABLE) {
    phys_addr_t region_start = base;
    phys_addr_t region_end = base + size;

    if (region_start < early_mem_end)
      region_start = early_mem_end;

    // Align start to next page
    region_start = (region_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (phys_addr_t paddr = region_start; paddr < region_end; paddr += PAGE_SIZE) {
      mark_page_free(paddr);
    }
  }
}

int pmm_register_region(uint64_t base, uint64_t len, pmm_region_type_t type,
                        uint32_t numa_node, uint32_t attrs) {
  (void)attrs;
  pmm_add_region((phys_addr_t)base, (size_t)len, type, numa_node);
  return 0;
}

int pmm_ingest_memory_map(const pmm_memory_map_t *map) {
  if (!map) return -1;
  for (uint32_t i = 0; i < map->region_count; i++) {
    pmm_register_region(map->regions[i].base_addr, map->regions[i].length, map->regions[i].type, map->regions[i].numa_node, map->regions[i].attributes);
  }
  return 0;
}

static bool g_pmm_initialized = false;
int mm_pmm_init(uint32_t magic, const boot_info_t *boot) {
  if (g_pmm_initialized) {
    return 0;
  }
  g_pmm_initialized = true;

  early_alloc_init(0);
  active_numa_nodes = 0U;

  pmm_memory_map_t map;
  map.region_count = 0;

  system_discovery_t *discovery = hal_get_system_discovery();
  if (discovery->topology.mem_region_count > 0) {
    for (uint32_t i = 0; i < discovery->topology.mem_region_count; i++) {
      if (discovery->topology.mem_regions[i].type == HAL_MEM_RAM) {
        if (map.region_count < MAX_PMM_REGIONS) {
          map.regions[map.region_count].base_addr =
              discovery->topology.mem_regions[i].base;
          map.regions[map.region_count].length =
              discovery->topology.mem_regions[i].size;
          map.regions[map.region_count].type = PMM_REGION_TYPE_USABLE;
          map.regions[map.region_count].numa_node =
              discovery->topology.mem_regions[i].node_id;
          map.region_count++;
        }
      }
    }
  }

  pmm_ingest_memory_map(&map);
  
  // Ensure page-table cache is available before VMM/hal_pt code consumes it.
  pt_cache_init();

  // Zero-initialize PStore region if it exists
  extern char _pstore_start[];
  extern char _pstore_end[];
  if ((uintptr_t)_pstore_start < (uintptr_t)_pstore_end) {
    size_t pstore_len = (size_t)(_pstore_end - _pstore_start);
    for (size_t i = 0; i < pstore_len; i++) {
      _pstore_start[i] = 0;
    }
  }

  KPRINT("PMM: ready\n");
  return 0;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
  pmm_block_t block;
  (void)preferred_numa_node;
  if (pmm_alloc_pages(0, PMM_ZONE_ANY, PMM_ALLOC_NONE, &block) == 0) {
    page_t *p = phys_to_page(block.phys_addr);
    if (p) p->owner_class = PMM_OWNER_CLASS_KERNEL;
    return block.phys_addr;
  }
  return 0;
}

int mm_alloc_dma_pages(size_t size, uint32_t preferred_numa_node,
                       uint32_t dma_flags, phys_addr_t *out_phys,
                       void **out_kernel_virt) {
  (void)preferred_numa_node;
  if (size == 0 || !out_phys || !out_kernel_virt) {
    return -1;
  }

  size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  pmm_block_t block;
  pmm_zone_t zone = (dma_flags & BHARAT_DMA_32BIT_ONLY) ? PMM_ZONE_DMA32 : PMM_ZONE_ANY;
  uint32_t alloc_flags = (dma_flags & BHARAT_DMA_ZERO) ? PMM_ALLOC_ZERO : PMM_ALLOC_NONE;

  if (pmm_alloc_contiguous((uint32_t)num_pages, zone, alloc_flags, &block) != 0) {
      return -1;
  }

  // Set class for all pages
  for (uint32_t i = 0; i < block.page_count; i++) {
      page_t *p = phys_to_page(block.phys_addr + i * PAGE_SIZE);
      if (p) p->owner_class = PMM_OWNER_CLASS_DMA;
  }

  *out_phys = block.phys_addr;
  *out_kernel_virt = (void *)(uintptr_t)block.phys_addr;

  return 0;
}

int mm_free_dma_pages(phys_addr_t phys, void *kernel_virt, size_t size) {
  (void)kernel_virt; // Assuming direct mapping

  if (phys == 0 || size == 0) {
    return -1;
  }

  size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  pmm_block_t block;
  block.phys_addr = phys;
  block.page_count = (uint32_t)num_pages;
  block.order = 0; // Contiguous block

  return pmm_free_pages(&block);
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
  size_t node_index =
      (size_t)((page_addr - numa_nodes[node_id].start_addr) / PAGE_SIZE);

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

    if ((uintptr_t)buddy < (uintptr_t)global_pages_ptrs[node_id] ||
        (uintptr_t)buddy >= (uintptr_t)(global_pages_ptrs[node_id] +
                                        numa_nodes[node_id].total_pages)) {
      hal_serial_write("PMM: CRITICAL: Buddy pointer out of bounds: ");
      hal_serial_write_hex((uintptr_t)buddy);
      hal_serial_write("\n");
      break;
    }

    if (buddy->ref_count > 0U || buddy->order != order) {
      break;
    }

    // Check if buddy is in free list
    int found = 0;
    uint32_t buddy_color = get_page_color(page_to_phys(buddy));
    list_head_t *pos;
    for (pos = zone->free_list[order][buddy_color].next;
         pos != &zone->free_list[order][buddy_color]; pos = pos->next) {
      if (pos == &buddy->list) {
        found = 1;
        break;
      }
    }
    if (!found) {
      break;
    }

    list_del(&buddy->list);
    zone->free_count[order][buddy_color]--;

    if (buddy_index < node_index) {
      page = buddy;
      node_index = buddy_index;
    }

    ++order;
  }

  page->order = order;
  page->ref_count = 0;
  page->flags = 0;

  uint32_t page_color = get_page_color(page_to_phys(page));
  list_add(&page->list, &zone->free_list[order][page_color]);
  zone->free_count[order][page_color]++;
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
void mm_inc_page_ref(phys_addr_t page_addr) { (void)page_addr; }
#endif
