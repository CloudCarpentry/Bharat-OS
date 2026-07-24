#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>

#include "../kernel/include/mm.h"
#include "../kernel/include/mm/pmm.h"
#include "../kernel/include/mm/pmm_map.h"
#include "../kernel/include/sched/sched.h"
#include "boot/boot_info.h"
#include "../kernel/include/hal/hal_discovery.h"
#include "../kernel/include/hal/hal_mm.h"

uint32_t hal_cpu_get_id(void) { return 0; }

static uint8_t g_mock_ram[64 * 1024 * 1024];
static size_t g_early_used = 0;
static system_discovery_t g_discovery;
char _pstore_start[1];
char _pstore_end[1];

void hal_serial_write(const char *s) { (void)s; }
void hal_serial_write_hex(uint64_t v) { (void)v; }
size_t string_length(const char *s) { return strlen(s); }
void console_write_raw(const char *s, size_t len) { (void)s; (void)len; }
void pt_cache_init(void) {}

void kernel_panic(const char *msg) {
    printf("KERNEL_PANIC: %s\n", msg);
    abort();
}

int mm_zero_phys_range(phys_addr_t phys, size_t len) { (void)phys; (void)len; return 0; }
void *physmap_phys_to_virt(phys_addr_t phys) { return (void *)(uintptr_t)phys; }

system_discovery_t *hal_get_system_discovery(void) { return &g_discovery; }
void early_alloc_init(size_t limit) { (void)limit; g_early_used = 0; }
void *early_alloc(size_t size, size_t align) {
  if (align > 1U) {
    size_t aligned = (g_early_used + align - 1U) & ~(align - 1U);
    g_early_used = aligned;
  }
  void *p = &g_mock_ram[g_early_used];
  g_early_used += size;
  return p;
}

void hal_mm_get_zone_limits(hal_mm_zone_limits_t *limits) {
  limits->dma_low_start = 0;
  limits->dma_low_end = 0;
  limits->dma32_start = (phys_addr_t)(uintptr_t)&g_mock_ram[0];
  limits->dma32_end = (phys_addr_t)(uintptr_t)&g_mock_ram[(1024 * PAGE_SIZE) - 1];
  limits->normal_start = (phys_addr_t)(uintptr_t)&g_mock_ram[1024 * PAGE_SIZE];
  limits->normal_end = (phys_addr_t)(uintptr_t)&g_mock_ram[sizeof(g_mock_ram) - 1];
  limits->flags = 0;
}

memory_node_id_t numa_get_current_node(void) { return 0; }
bh_thread_t *sched_current_thread(void) { return NULL; }
int sched_ai_apply_suggestion(const ai_suggestion_t *s) { (void)s; return 0; }

static void setup_pmm(void) {
  memset(&g_discovery, 0, sizeof(g_discovery));
  g_discovery.topology.mem_region_count = 1;
  g_discovery.topology.mem_regions[0].base = (uint64_t)(uintptr_t)&g_mock_ram[0];
  g_discovery.topology.mem_regions[0].size = sizeof(g_mock_ram);
  g_discovery.topology.mem_regions[0].type = HAL_MEM_RAM;
  g_discovery.topology.mem_regions[0].node_id = 0;

  struct boot_info bi = {0};
  assert(mm_pmm_init(0, &bi) == 0);
}

int main(void) {
  setup_pmm();

  // Exhaust the DMA32 window as far as PMM metadata/reservations allow.
  pmm_block_t dma_blocks[1024] = {0};
  size_t dma_alloc_count = 0;
  while (dma_alloc_count < 1024) {
    if (pmm_alloc_pages(0, PMM_ZONE_DMA32, PMM_ALLOC_NONE,
                        &dma_blocks[dma_alloc_count]) != 0) {
      break;
    }
    page_t *dp = phys_to_page(dma_blocks[dma_alloc_count].phys_addr);
    assert(dp != NULL);
    assert(dp->zone <= PMM_ZONE_DMA32);
    dma_alloc_count++;
  }
  assert(dma_alloc_count > 0);

  // With DMA32 exhausted and NORMAL pages still available, DMA32 request must
  // fail (must not leak a NORMAL page through fallback/retry paths).
  pmm_block_t exhausted_dma = {0};
  assert(pmm_alloc_pages(0, PMM_ZONE_DMA32, PMM_ALLOC_NONE, &exhausted_dma) != 0);

  pmm_block_t normal = {0};
  assert(pmm_alloc_pages(0, PMM_ZONE_NORMAL, PMM_ALLOC_NONE, &normal) == 0);
  page_t *np = phys_to_page(normal.phys_addr);
  assert(np != NULL);
  assert(np->zone <= PMM_ZONE_NORMAL);
  assert(pmm_free_pages(&normal) == 0);

  // Re-open exactly one DMA32 page and make sure allocation still honors zone.
  assert(pmm_free_pages(&dma_blocks[0]) == 0);
  pmm_block_t dma_retry = {0};
  assert(pmm_alloc_pages(0, PMM_ZONE_DMA32, PMM_ALLOC_NONE, &dma_retry) == 0);
  page_t *retry = phys_to_page(dma_retry.phys_addr);
  assert(retry != NULL);
  assert(retry->zone <= PMM_ZONE_DMA32);
  assert(pmm_free_pages(&dma_retry) == 0);

  for (size_t i = 1; i < dma_alloc_count; ++i) {
    assert(pmm_free_pages(&dma_blocks[i]) == 0);
  }

  printf("test_pmm_zone_alloc passed\n");
  return 0;
}
