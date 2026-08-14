#include "pmm_internal.h"

page_t *global_pages_ptrs[MAX_NUMA_NODES];
zone_t numa_zones[MAX_NUMA_NODES] __attribute__((aligned(64)));
numa_node_t numa_nodes[MAX_NUMA_NODES];
uint32_t active_numa_nodes = 0;

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

void pmm_reclaim_one_node(uint32_t node_id) {
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

bool page_block_matches_zone(page_t *base_page, int order, pmm_zone_t zone) {
  if (!base_page || zone == PMM_ZONE_ANY) {
    return true;
  }
  size_t page_count = (size_t)1U << order;
  for (size_t i = 0; i < page_count; i++) {
    if ((base_page + i)->zone > zone) {
      return false;
    }
  }
  return true;
}

static phys_addr_t pmm_alloc_pages_colored_in_zone(int order, uint32_t preferred_numa_node,
                                                   uint32_t flags,
                                                   mm_color_config_t *color_config,
                                                   pmm_zone_t zone_filter,
                                                   bool allow_numa_fallback);

static void *pmm_alloc_pages_order_colored(int order, uint32_t numa_node,
                                           mm_color_config_t *color_config,
                                           pmm_zone_t zone_filter) {
  uint32_t current_core = hal_cpu_get_id();
  pmm_core_state_t *core_state = NULL;

  if (current_core < BHARAT_MAX_CPUS) {
      core_state = &g_pmm_cores[current_core];
      core_state->active = true;
  }

  if (order == 0 && core_state && core_state->active && numa_node < 4) {
      hal_irq_state_t irq_state = hal_irq_save_disable();
      pmm_pcache_t *pcache = &core_state->node_caches[numa_node];
      if (pcache->count > 0) {
          phys_addr_t phys = pcache->pages[--pcache->count];
          pcache->alloc_hits++;
          hal_irq_restore(irq_state);

          page_t *page = phys_to_page(phys);
          if (page) {
              page->order = 0;
              bh_refcount_init(&page->ref_count, 1);
              page->flags = PAGE_FLAG_KERNEL;
              page->state = PMM_PAGE_STATE_ALLOCATED;
              page->owner_core_id = current_core;
          }
          return (void *)(uintptr_t)phys;
      } else {
          pcache->alloc_misses++;
          hal_irq_restore(irq_state);
      }
  }

  pmm_drain_remote_frees(current_core);
  if (order == 0 && core_state && core_state->active && numa_node < 4) {
      hal_irq_state_t irq_state = hal_irq_save_disable();
      pmm_pcache_t *pcache = &core_state->node_caches[numa_node];
      if (pcache->count > 0) {
          phys_addr_t phys = pcache->pages[--pcache->count];
          pcache->alloc_hits++;
          hal_irq_restore(irq_state);

          page_t *page = phys_to_page(phys);
          if (page) {
              page->order = 0;
              bh_refcount_init(&page->ref_count, 1);
              page->flags = PAGE_FLAG_KERNEL;
              page->state = PMM_PAGE_STATE_ALLOCATED;
              page->owner_core_id = current_core;
          }
          return (void *)(uintptr_t)phys;
      }
      hal_irq_restore(irq_state);
  }

  zone_t *zone = &numa_zones[numa_node];

  spin_lock(&zone->lock);

  if (order == 0 && core_state && core_state->active && numa_node < 4) {
      hal_irq_state_t irq_state = hal_irq_save_disable();
      pmm_pcache_t *pcache = &core_state->node_caches[numa_node];
      uint32_t refilled = 0;
      int start_color = 0;
      int end_color = CONFIG_MM_CACHE_COLORS_DEFAULT - 1;

      for (int c = start_color; c <= end_color && refilled < PMM_REFILL_BATCH; ++c) {
          while (!list_empty(&zone->free_list[0][c]) && 
                 refilled < PMM_REFILL_BATCH && 
                 pcache->count < PMM_PCACHE_HIGH) {
              page_t *page = list_entry(zone->free_list[0][c].next, page_t, list);
              list_del(&page->list);
              zone->free_count[0][c]--;
              pcache->pages[pcache->count++] = page_to_phys(page);
              refilled++;
          }
      }
      pcache->refill_count += refilled;

      if (pcache->count > 0) {
          phys_addr_t phys = pcache->pages[--pcache->count];
          pcache->alloc_hits++;
          hal_irq_restore(irq_state);
          spin_unlock(&zone->lock);

          page_t *page = phys_to_page(phys);
          if (page) {
              page->order = 0;
              bh_refcount_init(&page->ref_count, 1);
              page->flags = PAGE_FLAG_KERNEL;
              page->state = PMM_PAGE_STATE_ALLOCATED;
              page->owner_core_id = current_core;
          }
          return (void *)(uintptr_t)phys;
      }
      hal_irq_restore(irq_state);
  }

  if (core_state && core_state->active && numa_node < 4) {
      core_state->node_caches[numa_node].direct_zone_allocs++;
  }

  int target_color = -1;
  if (color_config && color_config->policy != MM_COLOR_POLICY_NONE) {
    for (int c = 0; c < CONFIG_MM_CACHE_COLORS_DEFAULT; ++c) {
      if ((color_config->color_mask & (1U << c)) != 0U) {
        if (!list_empty(&zone->free_list[order][c])) {
          target_color = c;
          break;
        }
      }
    }
    if (target_color == -1 && color_config->policy == MM_COLOR_POLICY_STRICT) {
      spin_unlock(&zone->lock);
      return NULL;
    }
  }

  for (int current_order = order; current_order < MAX_ORDER; ++current_order) {
    int start_color = (target_color != -1 && current_order == order) ? target_color : 0;
    int end_color = (target_color != -1 && current_order == order) ? target_color : (CONFIG_MM_CACHE_COLORS_DEFAULT - 1);

    for (int c = start_color; c <= end_color; ++c) {
      if (!list_empty(&zone->free_list[current_order][c])) {
        page_t *page = NULL;
        list_head_t *head = &zone->free_list[current_order][c];

        for (list_head_t *pos = head->next; pos != head; pos = pos->next) {
          page_t *candidate = list_entry(pos, page_t, list);
          if (page_block_matches_zone(candidate, current_order, zone_filter)) {
            page = candidate;
            list_del(&page->list);
            zone->free_count[current_order][c]--;
            break;
          }
        }

        if (!page) {
          continue;
        }

        while (current_order > order) {
          current_order--;
          size_t buddy_pfn = (size_t)1U << current_order;
          page_t *buddy = page + buddy_pfn;
          buddy->order = (int8_t)current_order;
          bh_refcount_init(&buddy->ref_count, 0);
          buddy->flags = 0;
          buddy->state = PMM_PAGE_STATE_FREE;
          buddy->pin_count = 0;
          uint32_t buddy_color = get_page_color(page_to_phys(buddy));
          list_add(&buddy->list, &zone->free_list[current_order][buddy_color]);
          zone->free_count[current_order][buddy_color]++;
        }

        page->order = (int8_t)order;
        bh_refcount_init(&page->ref_count, 1);
        page->flags = PAGE_FLAG_KERNEL;
        page->state = PMM_PAGE_STATE_ALLOCATED;
        page->owner_core_id = current_core;
        spin_unlock(&zone->lock);
        return (void *)(uintptr_t)page_to_phys(page);
      }
    }
  }

  spin_unlock(&zone->lock);
  return NULL;
}

int pmm_alloc_pages_ex(uint32_t order, pmm_zone_t zone, alloc_class_t cls, uint32_t alloc_flags, pmm_block_t *out_block) {
  if (!out_block) return -1;
  if (order >= MAX_ORDER) return -1;

  uint32_t numa_node = NUMA_NODE_ANY;
  uint32_t mm_flags = 0;
  if (alloc_flags & PMM_ALLOC_ZERO) mm_flags |= PMM_ALLOC_ZERO;

  phys_addr_t phys = pmm_alloc_pages_colored_in_zone(order, numa_node, mm_flags, NULL, zone, true);
  if (!phys) return -1;

  out_block->phys_addr = phys;
  out_block->order = (uint8_t)order;
  out_block->page_count = (1ULL << order);
  out_block->flags = alloc_flags;
  out_block->alloc_class = cls;

  if (alloc_flags & PMM_ALLOC_PINNED) {
    page_t *p = phys_to_page(phys);
    if (p) p->pin_count = 1;
  }

  return 0;
}

int pmm_alloc_contiguous_ex(uint32_t page_count, pmm_zone_t zone, alloc_class_t cls, uint32_t alloc_flags, pmm_block_t *out_block) {
  if (!out_block || page_count == 0) return -1;

  int order = 0;
  while ((1ULL << order) < page_count) {
    order++;
  }

  if ((1ULL << order) == page_count) {
    return pmm_alloc_pages_ex(order, zone, cls, alloc_flags, out_block);
  }

  if (order >= MAX_ORDER) return -1;

  pmm_block_t big_block;
  if (pmm_alloc_pages_ex(order, zone, cls, alloc_flags, &big_block) != 0) {
    return -1;
  }

  phys_addr_t base_phys = big_block.phys_addr;
  uint32_t allocated_pages = (1ULL << big_block.order);

  out_block->phys_addr = base_phys;
  out_block->order = 0;
  out_block->page_count = page_count;
  out_block->flags = alloc_flags;
  out_block->alloc_class = cls;

  for (uint32_t i = page_count; i < allocated_pages; i++) {
    phys_addr_t free_phys = base_phys + i * PAGE_SIZE;
    page_t *p = phys_to_page(free_phys);
    if (p) {
        bh_refcount_init(&p->ref_count, 1);
        p->order = 0;
    }
    mm_free_page(free_phys);
  }

  page_t *head_p = phys_to_page(base_phys);
  if (head_p) {
      head_p->order = 0;
      head_p->pin_count = (alloc_flags & PMM_ALLOC_PINNED) ? 1 : 0;
  }

  return 0;
}

int pmm_free_pages(const pmm_block_t *block) {
  if (!block) return -1;
  phys_addr_t phys = block->phys_addr;

  if (block->page_count > 0 && block->page_count != (1ULL << block->order)) {
    // Phase 1: validate entire range
    for (uint32_t i = 0; i < block->page_count; i++) {
      phys_addr_t p = phys + i * PAGE_SIZE;
      page_t *page = phys_to_page(p);
      if (page) {
          if (page->pin_count > 0) return -1;
      }
    }
    // Phase 2: mutate/free entire range
    for (uint32_t i = 0; i < block->page_count; i++) {
      phys_addr_t p = phys + i * PAGE_SIZE;
      page_t *page = phys_to_page(p);
      if (page) {
          page->order = 0;
          bh_refcount_init(&page->ref_count, 1);
      }
      mm_free_page(p);
    }
    return 0;
  } else {
      page_t *page = phys_to_page(phys);
      if (page) {
          if (page->pin_count > 0) return -1;
      }
      if (page) {
          page->order = (int8_t)block->order;
          bh_refcount_init(&page->ref_count, 1);
      }
      mm_free_page(phys);
      return 0;
  }
}

int pmm_ref_get(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (page) {
        return (pmm_page_get(page) == K_OK) ? 0 : -1;
    }
    return -1;
}

int pmm_ref_put(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (!page) return -1;

    bool is_last = false;
    kstatus_t status = pmm_page_put(page, &is_last);
    if (status == K_OK) {
        if (is_last) {
            mm_free_page(phys_addr);
        }
        return 0;
    }
    return -1;
}

int pmm_pin(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (!page) return -1;
    __atomic_fetch_add(&page->pin_count, 1U, __ATOMIC_SEQ_CST);
    return 0;
}

int pmm_unpin(uint64_t phys_addr) {
    page_t *page = phys_to_page(phys_addr);
    if (!page) return -1;
    while (1) {
        uint16_t old = __atomic_load_n(&page->pin_count, __ATOMIC_ACQUIRE);
        if (old == 0U) {
            return -1;
        }
        if (__atomic_compare_exchange_n(&page->pin_count, &old, (uint16_t)(old - 1U), false, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)) {
            return 0;
        }
    }
}

static phys_addr_t pmm_alloc_pages_colored_in_zone(int order, uint32_t preferred_numa_node,
                                                   uint32_t flags,
                                                   mm_color_config_t *color_config,
                                                   pmm_zone_t zone_filter,
                                                   bool allow_numa_fallback) {
  uint32_t home = preferred_numa_node;

  if (preferred_numa_node == NUMA_NODE_ANY ||
      preferred_numa_node >= active_numa_nodes) {
    memory_node_id_t current = numa_get_current_node();
    home = (current < active_numa_nodes) ? current : 0U;
  }

  uint32_t node_attempts = allow_numa_fallback ? active_numa_nodes : 1U;
  for (uint32_t attempt = 0; attempt < node_attempts; ++attempt) {
    uint32_t node_id = (home + attempt) % active_numa_nodes;

    if (numa_nodes[node_id].free_pages < (1ULL << order)) {
      pmm_reclaim_one_node(node_id);
      if (numa_nodes[node_id].free_pages < (1ULL << order))
        continue;
    }

    void *addr = pmm_alloc_pages_order_colored(order, node_id, color_config, zone_filter);
    if (addr) {
      atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages,
                                 (1ULL << order));
      page_t *page = phys_to_page((phys_addr_t)(uintptr_t)addr);
      if (page) {
        page->flags = flags;
      }

      if (flags & PMM_ALLOC_ZERO) {
          if (physmap_has_linear_map()) {
              void *va = physmap_phys_to_virt((phys_addr_t)(uintptr_t)addr);
              if (va) {
                  memset(va, 0, (1ULL << order) * PAGE_SIZE);
              }
          } else {
              uint8_t *p = (uint8_t*)(uintptr_t)addr;
              size_t total = (1ULL << order) * PAGE_SIZE;
              for (size_t i = 0; i < total; i++) p[i] = 0;
          }
      }

      return (phys_addr_t)(uintptr_t)addr;
    }
  }

  if (color_config && color_config->policy == MM_COLOR_POLICY_PREFERRED) {
    for (uint32_t attempt = 0; attempt < node_attempts; ++attempt) {
      uint32_t node_id = (home + attempt) % active_numa_nodes;
      mm_color_config_t no_color_config = {.policy = MM_COLOR_POLICY_NONE,
                                           .domain = MM_DOMAIN_DEFAULT,
                                           .color_mask = 0xFFFFFFFF};
      void *addr =
          pmm_alloc_pages_order_colored(order, node_id, &no_color_config, zone_filter);
      if (addr) {
        atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages,
                                   (1ULL << order));
        page_t *page = phys_to_page((phys_addr_t)(uintptr_t)addr);
        if (page) {
          page->flags = flags;
        }

        if (flags & PMM_ALLOC_ZERO) {
            void *va = physmap_phys_to_virt((phys_addr_t)(uintptr_t)addr);
            if (va) {
                memset(va, 0, (1ULL << order) * PAGE_SIZE);
            }
        }

        return (phys_addr_t)(uintptr_t)addr;
      }
    }
  }

  for (uint32_t attempt = 0; attempt < node_attempts; ++attempt) {
    pmm_reclaim_one_node((home + attempt) % active_numa_nodes);
  }

  for (uint32_t attempt = 0; attempt < node_attempts; attempt++) {
    uint32_t node_id = (home + attempt) % active_numa_nodes;
    void *addr = pmm_alloc_pages_order_colored(order, node_id, color_config, zone_filter);
    if (addr) {
      atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages,
                                 (1ULL << order));
      page_t *page = phys_to_page((phys_addr_t)(uintptr_t)addr);
      if (page) {
        page->flags = flags;
      }

      if (flags & PMM_ALLOC_ZERO) {
          if (physmap_has_linear_map()) {
              void *va = physmap_phys_to_virt((phys_addr_t)(uintptr_t)addr);
              if (va) {
                  memset(va, 0, (1ULL << order) * PAGE_SIZE);
              }
          } else {
              uint8_t *p = (uint8_t*)(uintptr_t)addr;
              size_t total = (1ULL << order) * PAGE_SIZE;
              for (size_t i = 0; i < total; i++) p[i] = 0;
          }
      }

      return (phys_addr_t)(uintptr_t)addr;
    }
  }



  return 0;
}

phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node,
                                 uint32_t flags) {
  bh_thread_t *current = sched_current_thread();
  mm_color_config_t *color_config = NULL;
  if (current) {
    color_config = &current->mm_color_policy;
  }
  return pmm_alloc_pages_colored(order, preferred_numa_node, flags, color_config);
}

phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node,
                                    uint32_t flags,
                                    mm_color_config_t *color_config) {
  return pmm_alloc_pages_colored_in_zone(order, preferred_numa_node, flags,
                                         color_config, PMM_ZONE_ANY, true);
}

phys_addr_t pmm_alloc_page_node(memory_node_id_t node, int strict) {
  bh_thread_t *current = sched_current_thread();
  mm_color_config_t *colors = current ? &current->mm_color_policy : NULL;
  if (active_numa_nodes == 0U || node >= active_numa_nodes) {
    return strict ? 0U : mm_alloc_page(NUMA_NODE_ANY);
  }
  return pmm_alloc_pages_colored_in_zone(0, node, PAGE_FLAG_KERNEL, colors,
                                         PMM_ZONE_ANY, strict == 0);
}

void mark_page_free(phys_addr_t phys) {
  page_t *p = phys_to_page(phys);
  if (!p) {
    return;
  }
  bh_refcount_init(&p->ref_count, 1);
  p->order = 0;
  p->state = PMM_PAGE_STATE_ALLOCATED;
  mm_free_page(phys);
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
  pmm_block_t block;
  (void)preferred_numa_node;
  if (pmm_alloc_pages_ex(0, PMM_ZONE_ANY, MEM_NORMAL, PMM_ALLOC_NONE, &block) == 0) {
    page_t *p = phys_to_page(block.phys_addr);
    if (p) p->owner_class = (uint8_t)PMM_OWNER_CLASS_KERNEL;
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

  if (pmm_alloc_contiguous_ex((uint32_t)num_pages, zone, MEM_DMA, alloc_flags, &block) != 0) {
      return -1;
  }

  for (uint32_t i = 0; i < block.page_count; i++) {
      page_t *p = phys_to_page(block.phys_addr + i * PAGE_SIZE);
      if (p) p->owner_class = (uint8_t)PMM_OWNER_CLASS_DMA;
  }

  *out_phys = block.phys_addr;
  *out_kernel_virt = physmap_phys_to_virt(block.phys_addr);
  if (!*out_kernel_virt) {
    (void)pmm_free_pages(&block);
    return -1;
  }

  return 0;
}

int mm_free_dma_pages(phys_addr_t phys, void *kernel_virt, size_t size) {
  (void)kernel_virt;

  if (phys == 0 || size == 0) {
    return -1;
  }

  size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  pmm_block_t block;
  block.phys_addr = phys;
  block.page_count = (uint32_t)num_pages;
  block.order = 0;

  return pmm_free_pages(&block);
}

static void __mm_free_page(phys_addr_t page_addr, bool bypass_pcache) {
  page_t *page = phys_to_page(page_addr);
  if (!page) {
    return;
  }

  if (page->state == PMM_PAGE_STATE_FREE) {
    hal_serial_write("PMM: Double free of phys: ");
    hal_serial_write_hex(page_addr);
    hal_serial_write(" from PC: ");
    hal_serial_write_hex((uintptr_t)__builtin_return_address(0));
    hal_serial_write("\n");
    kernel_panic("PMM: Double free detected!\n");
  }

  uint32_t observed = bh_refcount_read(&page->ref_count);
  if (observed > 0U) {
    bool is_last = false;
    kstatus_t status = bh_refcount_dec_and_test(&page->ref_count, &is_last);
    if (status != K_OK || !is_last) {
      return;
    }
  } else {
    uint32_t expected = 0;
    if (!__atomic_compare_exchange_n(&page->ref_count.value, &expected, 0U, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        return;
    }
  }

  if (physmap_has_linear_map()) {
      void *va = physmap_phys_to_virt(page_addr);
      if (va) {
          uint8_t *ptr = (uint8_t *)va;
          for (size_t i = 0; i < PAGE_SIZE; i++) {
              ptr[i] = 0xAA;
          }
      }
  }

  uint32_t node_id = page->numa_node;
  int original_order = page->order;
  int order = original_order;
  if (order < 0) {
    original_order = 0;
    order = 0;
  }

  uint32_t current_core = hal_cpu_get_id();
  pmm_core_state_t *core_state = NULL;

  if (current_core < BHARAT_MAX_CPUS) {
      core_state = &g_pmm_cores[current_core];
  }

  if (order == 0 && !bypass_pcache && core_state && core_state->active && pmm_numa_node_valid(node_id) && pmm_page_can_enter_pcache(page)) {
      if (pmm_page_owned_by_core(page, current_core)) {
          hal_irq_state_t irq_state = hal_irq_save_disable();
          pmm_pcache_t *pcache = &core_state->node_caches[node_id];

          if (pcache->count < PMM_PCACHE_HIGH) {
              pcache->pages[pcache->count++] = page_addr;
              pcache->local_frees++;
              page->state = PMM_PAGE_STATE_FREE;
              bh_refcount_init(&page->ref_count, 0);
              page->flags = 0;
              page->pin_count = 0;
              page->order = 0;
              hal_irq_restore(irq_state);
              atomic64_fetch_and_add_ptr(&numa_nodes[node_id].free_pages, 1ULL);
              return;
          } else {
              pcache->drain_to_zone_count++;

              for (uint32_t i = 0; i < PMM_DRAIN_BATCH; i++) {
                  phys_addr_t drain_phys = pcache->pages[--pcache->count];
                  page_t *drain_page = phys_to_page(drain_phys);

                  if (drain_page) {
                      bh_refcount_init(&drain_page->ref_count, 1);
                      drain_page->state = PMM_PAGE_STATE_ALLOCATED;
                      atomic64_fetch_and_sub_ptr(&numa_nodes[drain_page->numa_node].free_pages, 1ULL);
                      __mm_free_page(drain_phys, true);
                  }
              }

              pcache->pages[pcache->count++] = page_addr;
              pcache->local_frees++;
              page->state = PMM_PAGE_STATE_FREE;
              bh_refcount_init(&page->ref_count, 0);
              page->flags = 0;
              page->pin_count = 0;
              page->order = 0;
              hal_irq_restore(irq_state);
              atomic64_fetch_and_add_ptr(&numa_nodes[node_id].free_pages, 1ULL);
              return;
          }
      } else if (pmm_core_id_valid(page->owner_core_id) && g_pmm_cores[page->owner_core_id].active) {
          if (pmm_pcache_remote_free_enqueue(page->owner_core_id, page_addr) == K_OK) {
              page->state = PMM_PAGE_STATE_REMOTE_FREE_PENDING;
              bh_refcount_init(&page->ref_count, 0);
              page->flags = 0;
              page->pin_count = 0;
              page->order = 0;
              return;
          }
      }
  }

  if (core_state && core_state->active && node_id < 4) {
      core_state->node_caches[node_id].direct_zone_frees++;
  }

  zone_t *zone = &numa_zones[node_id];
  size_t node_index =
      (size_t)((page_addr - numa_nodes[node_id].start_addr) / PAGE_SIZE);

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

    if (bh_refcount_read(&buddy->ref_count) > 0U || buddy->order != order) {
      break;
    }

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

  page->order = (int8_t)order;
  bh_refcount_init(&page->ref_count, 0);
  page->flags = 0;
  page->state = PMM_PAGE_STATE_FREE;
  page->pin_count = 0;

  uint32_t page_color = get_page_color(page_to_phys(page));
  list_add(&page->list, &zone->free_list[order][page_color]);
  zone->free_count[order][page_color]++;
  spin_unlock(&zone->lock);

  atomic64_fetch_and_add_ptr(&numa_nodes[node_id].free_pages, (1ULL << original_order));
}

void mm_free_page(phys_addr_t page_addr) {
    __mm_free_page(page_addr, false);
}

#ifndef Profile_RTOS
void mm_inc_page_ref(phys_addr_t page_addr) {
  page_t *page = phys_to_page(page_addr);
  if (page) {
    (void)pmm_page_get(page);
  }
}
#else
void mm_inc_page_ref(phys_addr_t page_addr) { (void)page_addr; }
#endif

void *pmm_alloc_page_ex(alloc_class_t cls, uint32_t flags) {
    pmm_block_t block;
    if (pmm_alloc_pages_ex(0, PMM_ZONE_ANY, cls, flags, &block) == 0) {
        return (void*)block.phys_addr;
    }
    return NULL;
}

void *pmm_alloc_zeroed_page_ex(alloc_class_t cls, uint32_t flags) {
    return pmm_alloc_page_ex(cls, flags | PMM_ALLOC_ZERO);
}

void *pmm_alloc_contig_ex(size_t npages, size_t align_pages, alloc_class_t cls, uint32_t flags) {
    (void)align_pages;
    pmm_block_t block;
    if (pmm_alloc_contiguous_ex((uint32_t)npages, PMM_ZONE_ANY, cls, flags, &block) == 0) {
        return (void*)block.phys_addr;
    }
    return NULL;
}

void pmm_free_page(void *page) {
    pmm_block_t block;
    block.phys_addr = (uintptr_t)page;
    block.page_count = 1;
    block.order = 0;
    pmm_free_pages(&block);
}

void page_get(page_t *page) {
    if (page) {
        (void)pmm_page_get(page);
    }
}

void page_put(page_t *page) {
    if (page) {
        pmm_ref_put(page_to_phys((page_t*)page));
    }
}

bool page_try_get(page_t *page) {
    if (!page) return false;
    return pmm_page_get(page) == K_OK;
}
