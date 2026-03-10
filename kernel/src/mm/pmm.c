#include "../../include/atomic.h"
#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/profile.h"
#include "../../include/spinlock.h"

#include "../../include/sched.h"
#include "early_alloc.h"

// Multiboot only for x86_64
#if defined(__x86_64__)
#include "../boot/x86_64/multiboot2.h"
#endif

#include "../../include/hal/hal.h"
#define KPRINT(s) hal_serial_write(s)

#include <stddef.h>
#include <stdint.h>

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

#if defined(__x86_64__)
#define MULTIBOOT1_BOOTLOADER_MAGIC 0x2BADB002
#endif

/* Multiboot 1 Structures for legacy support */
typedef struct {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t syms[4];
  uint32_t mmap_length;
  uint32_t mmap_addr;
} mb1_info_t;

typedef struct {
  uint32_t size;
  uint64_t addr;
  uint64_t len;
  uint32_t type;
} __attribute__((packed)) mb1_mmap_entry_t;

/*
 * Multi-Architecture PMM Discovery Logic
 * Supports Multiboot (x86), Device Tree (ARM/RISC-V), and Fixed Maps (Cortex-M)
 */

static void pmm_add_region(phys_addr_t base, size_t size) {
  if (size == 0)
    return;

  KPRINT("PMM: Adding region: base=");
  // Printing hex manually since KPRINT is serial only
  hal_serial_write_hex(base);
  KPRINT(", size=");
  hal_serial_write_hex(size);
  KPRINT("\n");

  KPRINT("PMM: Adding region: base=");
  hal_serial_write_hex(base);
  KPRINT(", size=");
  hal_serial_write_hex(size);
  KPRINT("\n");

  uint32_t page_count = (uint32_t)(size / PAGE_SIZE);
  phys_addr_t early_mem_end =
      (early_alloc_get_current_ptr() + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

  KPRINT("PMM: early_mem_end=");
  hal_serial_write_hex(early_mem_end);
  KPRINT("\n");

  if (active_numa_nodes >= MAX_NUMA_NODES)
    return;
  uint32_t node_id = active_numa_nodes++;

  numa_nodes[node_id].node_id = node_id;
  numa_nodes[node_id].start_addr = base;
  numa_nodes[node_id].total_pages = page_count;
  numa_nodes[node_id].free_pages = 0;
  numa_nodes[node_id].allocator_metadata = &numa_zones[node_id];

  size_t page_array_size = page_count * sizeof(page_t);
  global_pages_ptrs[node_id] =
      (page_t *)early_alloc(page_array_size, PAGE_SIZE);

  zone_t *zone = &numa_zones[node_id];
  spin_lock_init(&zone->lock);
  zone->reclaim_count = 0U;

  for (int o = 0; o < MAX_ORDER; o++) {
    for (int c = 0; c < CONFIG_MM_CACHE_COLORS_DEFAULT; c++) {
      list_init(&zone->free_list[o][c]);
      zone->free_count[o][c] = 0;
    }
  }

  // Initialize page metadata
  for (size_t j = 0; j < page_count; j++) {
    page_t *p = &global_pages_ptrs[node_id][j];
    p->ref_count = 1;
    p->numa_node = node_id;
    p->flags = PAGE_FLAG_RESERVED;
    p->order = -1;
    list_init(&p->list);
  }

  // Second pass: free available pages into the buddy allocator
  phys_addr_t region_start = (base + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  phys_addr_t region_end = (base + size) & ~(PAGE_SIZE - 1);
  if (region_start < early_mem_end)
    region_start = early_mem_end;

  for (phys_addr_t p = region_start; p < region_end; p += PAGE_SIZE) {
    mark_page_free(p);
  }
}

static int pmm_discovery_multiboot(uint32_t magic, void *memory_map) {
#if defined(__x86_64__)
  if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
    multiboot_information_t *mb_info = (multiboot_information_t *)memory_map;
    uint32_t total_size = mb_info->total_size;
    uint8_t *tag_ptr = (uint8_t *)mb_info + 8;
    while (tag_ptr < (uint8_t *)mb_info + total_size) {
      multiboot_tag_t *tag = (multiboot_tag_t *)tag_ptr;
      if (tag->type == MULTIBOOT_TAG_TYPE_END)
        break;
      if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
        multiboot_tag_mmap_t *mmap_tag = (multiboot_tag_mmap_t *)tag;
        uint32_t entry_size =
            (mmap_tag->entry_size < sizeof(multiboot_mmap_entry_t))
                ? sizeof(multiboot_mmap_entry_t)
                : mmap_tag->entry_size;
        uint32_t num_entries =
            (mmap_tag->size > 16) ? (mmap_tag->size - 16) / entry_size : 0;
        for (uint32_t i = 0; i < num_entries; i++) {
          multiboot_mmap_entry_t *entry =
              (multiboot_mmap_entry_t *)((uint8_t *)mmap_tag->entries +
                                         (i * entry_size));
          if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            pmm_add_region(entry->addr, entry->len);
          }
        }
      }
      tag_ptr += (tag->size + 7) & ~7;
    }
    return 0;
  } else if (magic == MULTIBOOT1_BOOTLOADER_MAGIC) {
    mb1_info_t *mb_info = (mb1_info_t *)memory_map;
    if (!(mb_info->flags & (1 << 6)))
      return -1;
    mb1_mmap_entry_t *mmap = (mb1_mmap_entry_t *)(uintptr_t)mb_info->mmap_addr;
    uint32_t mmap_length = mb_info->mmap_length;
    for (uint32_t offset = 0; offset < mmap_length;) {
      mb1_mmap_entry_t *entry = (mb1_mmap_entry_t *)((uint8_t *)mmap + offset);
      if (entry->type == 1) {
        pmm_add_region(entry->addr, entry->len);
      }
      offset += entry->size + 4;
    }
    return 0;
  }
#endif
  (void)magic;
  (void)memory_map;
  return -1;
}

static int pmm_discovery_fixed(void) {
#if defined(__riscv)
  pmm_add_region(0x80000000ULL, 0x8000000ULL); // 128MB @ 0x80000000 (QEMU Virt)
  return 0;
#elif defined(__aarch64__) || defined(__arm__)
  pmm_add_region(0x40000000ULL, 0x8000000ULL); // 128MB @ 0x40000000 (QEMU Virt)
  return 0;
#elif defined(__x86_64__) || defined(__i386__)
  pmm_add_region(0x1000000ULL, 0x8000000ULL); // 128MB Fallback
  return 0;
#endif
  return -1;
}

int mm_pmm_init(uint32_t magic, void *memory_map) {
  early_alloc_init(0);
  active_numa_nodes = 0U;

  if (pmm_discovery_multiboot(magic, memory_map) == 0) {
    KPRINT("PMM: initialized via Multiboot\n");
  } else if (pmm_discovery_fixed() == 0) {
    KPRINT("PMM: initialized via Fixed Map fallback\n");
  } else {
    return -1;
  }

  // Zero-initialize PStore region if it exists
  extern char _pstore_start[];
  extern char _pstore_end[];
  if (_pstore_start < _pstore_end) {
    size_t pstore_len = (size_t)(_pstore_end - _pstore_start);
    for (size_t i = 0; i < pstore_len; i++) {
      _pstore_start[i] = 0;
    }
  }

  KPRINT("PMM: ready\n");
  return 0;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
  return mm_alloc_pages_order(0, preferred_numa_node, PAGE_FLAG_KERNEL);
}

int mm_alloc_dma_pages(size_t size, uint32_t preferred_numa_node,
                       uint32_t dma_flags, phys_addr_t *out_phys,
                       void **out_kernel_virt) {
  if (size == 0 || !out_phys || !out_kernel_virt) {
    return -1;
  }

  // Calculate required order
  size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  int order = 0;
  while ((1ULL << order) < num_pages) {
    order++;
  }

  if (order >= MAX_ORDER) {
    return -1;
  }

  phys_addr_t phys = mm_alloc_pages_order(order, preferred_numa_node, PAGE_FLAG_KERNEL);
  if (phys == 0) {
    return -1;
  }

  if (dma_flags & BHARAT_DMA_32BIT_ONLY) {
    if (phys + size > 0xFFFFFFFFULL) {
      page_t *p = phys_to_page(phys);
      if (p) {
        p->order = order;
      }
      mm_free_page(phys);
      return -1;
    }
  }

  void *virt = (void *)(uintptr_t)phys; // Direct mapping assumption for simple implementation

  if (dma_flags & BHARAT_DMA_ZERO) {
    uint8_t *ptr = (uint8_t *)virt;
    for (size_t i = 0; i < (1ULL << order) * PAGE_SIZE; i++) {
      ptr[i] = 0;
    }
  }

  *out_phys = phys;
  *out_kernel_virt = virt;

  // Cache/Coherency flags to be passed to hal mapping paths if we managed our own vmap
  // For now we assume direct mapped kernel memory handles cacheability per region / via PAT/PMA later
  (void)dma_flags;

  return 0;
}

int mm_free_dma_pages(phys_addr_t phys, void *kernel_virt, size_t size) {
  (void)kernel_virt; // Assuming direct mapping

  if (phys == 0 || size == 0) {
    return -1;
  }

  size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  int order = 0;
  while ((1ULL << order) < num_pages) {
    order++;
  }

  page_t *p = phys_to_page(phys);
  if (p) {
    p->order = order;
  }

  mm_free_page(phys);
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
