/*
 * PMM Internal Logic
 * Not to be exposed to vm/aspace or hal/
 */
#ifndef PMM_INTERNAL_H
#define PMM_INTERNAL_H

#include "mm/pmm.h"
#include "spinlock.h"
#include "atomic.h"
#include "lib/base/string.h"
#include "mm.h"
#include "numa.h"
#include "profile/profile.h"
#include "panic.h"
#include "sched/sched.h"
#include "early_alloc.h"
#include "boot/boot_info.h"
#include "boot/boot_contract.h"
#include "hal/hal.h"
#include "hal/hal_discovery.h"
#include "hal/hal_mm.h"
#include "console/console_core.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "mm/pmm_map.h"
#include "mm/pt_cache.h"
#include "mm/physmap.h"
#include "mm/pmm_pcache.h"
#include "pmm_lifecycle.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

#define MAX_ORDER 12
#define MAX_NUMA_NODES 4
#define PMM_RECLAIM_BATCH 32U
#define PMM_LOW_WATERMARK_PAGES 128U
#define MAX_PMM_BOOT_RESERVATIONS 16

typedef struct {
    phys_addr_t start;
    phys_addr_t end;       /* exclusive */
    uint32_t type;
    bool releasable;
} pmm_boot_reserved_range_t;

typedef struct __attribute__((aligned(64))) {
  spinlock_t lock;
  list_head_t free_list[MAX_ORDER][CONFIG_MM_CACHE_COLORS_DEFAULT];
  size_t free_count[MAX_ORDER][CONFIG_MM_CACHE_COLORS_DEFAULT];
  phys_addr_t reclaim_pool[PMM_RECLAIM_BATCH];
  uint32_t reclaim_count;
} zone_t;

extern page_t *global_pages_ptrs[MAX_NUMA_NODES];
extern zone_t numa_zones[MAX_NUMA_NODES];
extern numa_node_t numa_nodes[MAX_NUMA_NODES];
extern uint32_t active_numa_nodes;
extern bool g_pmm_initialized;

phys_addr_t page_to_phys(page_t *page);
page_t *phys_to_page(phys_addr_t phys);

static inline uint32_t get_page_color(phys_addr_t phys) {
  return (phys / PAGE_SIZE) % CONFIG_MM_CACHE_COLORS_DEFAULT;
}

void pmm_boot_reservations_init(const boot_info_t *boot);
bool pmm_boot_page_is_reserved(phys_addr_t paddr);
phys_addr_t pmm_boot_reservation_end(phys_addr_t paddr);
void pmm_add_region(phys_addr_t base, size_t size, uint32_t type, uint32_t target_numa_node);
void mark_page_free(phys_addr_t phys);
void pmm_reclaim_one_node(uint32_t node_id);
bool page_block_matches_zone(page_t *base_page, int order, pmm_zone_t zone);

#endif /* PMM_INTERNAL_H */
