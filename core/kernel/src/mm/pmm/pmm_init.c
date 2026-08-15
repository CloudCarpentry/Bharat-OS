#include "pmm_internal.h"

static pmm_boot_reserved_range_t boot_reservations[MAX_PMM_BOOT_RESERVATIONS];
static uint32_t boot_reservation_count = 0;

void pmm_boot_reservations_init(const boot_info_t *boot) {
    if (!boot) return;

    if (boot->kernel_phys_start < boot->kernel_phys_end) {
        if (boot_reservation_count < MAX_PMM_BOOT_RESERVATIONS) {
            boot_reservations[boot_reservation_count].start = boot->kernel_phys_start & ~0xFFFULL;
            boot_reservations[boot_reservation_count].end = (boot->kernel_phys_end + 0xFFFULL) & ~0xFFFULL;
            boot_reservations[boot_reservation_count].type = PMM_REGION_TYPE_RESERVED;
            boot_reservations[boot_reservation_count].releasable = false;
            boot_reservation_count++;
        }
    }

    for (uint32_t i = 0; i < boot->module_count; ++i) {
        if (boot_reservation_count < MAX_PMM_BOOT_RESERVATIONS) {
            phys_addr_t m_start = boot->modules[i].phys_start & ~0xFFFULL;
            phys_addr_t m_end = (boot->modules[i].phys_start + boot->modules[i].size + 0xFFFULL) & ~0xFFFULL;
            boot_reservations[boot_reservation_count].start = m_start;
            boot_reservations[boot_reservation_count].end = m_end;
            boot_reservations[boot_reservation_count].type = PMM_REGION_TYPE_MODULES;
            boot_reservations[boot_reservation_count].releasable = false;
            boot_reservation_count++;

            KPRINT("PMM_RES: MOD start=");
            for (int k = 7; k >= 0; k--) {
                uint32_t nib = (m_start >> (k * 4)) & 0xF;
                char c = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
                char buf[2] = {c, '\0'};
                KPRINT(buf);
            }
            KPRINT(" end=");
            for (int k = 7; k >= 0; k--) {
                uint32_t nib = (m_end >> (k * 4)) & 0xF;
                char c = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
                char buf[2] = {c, '\0'};
                KPRINT(buf);
            }
            KPRINT("\n");
        }
    }
}

bool pmm_boot_page_is_reserved(phys_addr_t paddr) {
    for (uint32_t i = 0; i < boot_reservation_count; ++i) {
        if (paddr >= boot_reservations[i].start && paddr < boot_reservations[i].end) {
            return true;
        }
    }
    return false;
}

phys_addr_t pmm_boot_reservation_end(phys_addr_t paddr) {
    for (uint32_t i = 0; i < boot_reservation_count; ++i) {
        if (paddr >= boot_reservations[i].start && paddr < boot_reservations[i].end) {
            return boot_reservations[i].end;
        }
    }
    return 0;
}

void pmm_add_region(phys_addr_t base, size_t size, uint32_t type,
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
    bh_refcount_init(&p->ref_count, 1);
    p->numa_node = (uint8_t)node_id;
    p->flags = 0;
    p->order = 0; // Initialize order to 0 so buddy merging works immediately
    p->state = PMM_PAGE_STATE_RESERVED; // Default to reserved
    p->pin_count = 0;
    list_init(&p->list);

    phys_addr_t paddr = base + (j * PAGE_SIZE);
    if (paddr >= limits.dma32_start && paddr <= limits.dma32_end) {
      p->zone = (uint8_t)PMM_ZONE_DMA32;
    } else {
      p->zone = (uint8_t)PMM_ZONE_NORMAL;
    }
  }

  // Pass 2: Free usable RAM pages (skipping reserved early memory)
  if (type == PMM_REGION_TYPE_USABLE) {
    phys_addr_t region_start = base;
    phys_addr_t region_end = base + size;

    if (region_start < early_mem_end) {
      region_start = early_mem_end;
    }

    region_start = (region_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (phys_addr_t paddr = region_start; paddr < region_end; paddr += PAGE_SIZE) {
      if (pmm_boot_page_is_reserved(paddr)) {
          continue;
      }
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

bool g_pmm_initialized = false;
int mm_pmm_init(uint32_t magic, const boot_info_t *boot) {
  (void)magic;

  if (g_pmm_initialized) {
    return 0;
  }
  g_pmm_initialized = true;

  pmm_boot_reservations_init(boot);

  early_alloc_init(0);

  if (mem_model_validate_hal_caps(mem_model_get_current(), hal_memory_caps()) != K_OK) {
    kernel_panic("PMM: Memory model requested by profile is not supported by HAL/Hardware!");
  }

  pmm_pcache_init_all();
  active_numa_nodes = 0U;

  pmm_memory_map_t map;
  for (uint32_t i = 0; i < MAX_PMM_REGIONS; i++) {
      map.regions[i].base_addr = 0;
      map.regions[i].length = 0;
  }
  map.region_count = 0;

  system_discovery_t *discovery = hal_get_system_discovery();
  if (discovery->topology.mem_region_count > 0) {
    for (uint32_t i = 0; i < discovery->topology.mem_region_count; i++) {
      if (discovery->topology.mem_regions[i].type == 1) { // HAL_MEM_RAM
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

  if (map.region_count == 0 && boot && boot->mem_region_count > 0) {
    for (uint32_t i = 0; i < boot->mem_region_count; i++) {
      if (boot->mem_regions[i].type == BOOT_MEM_USABLE) {
        if (map.region_count < MAX_PMM_REGIONS) {
          map.regions[map.region_count].base_addr = boot->mem_regions[i].phys_start;
          map.regions[map.region_count].length = boot->mem_regions[i].size;
          map.regions[map.region_count].type = PMM_REGION_TYPE_USABLE;
          map.regions[map.region_count].numa_node = 0;
          map.region_count++;
        }
      }
    }
  }

  pmm_ingest_memory_map(&map);
  
  pt_cache_init();

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
