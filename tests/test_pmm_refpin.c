#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../kernel/include/mm.h"
#include "../kernel/include/mm/pmm.h"
#include "../kernel/include/sched.h"
#include "../kernel/include/bharat/boot_info.h"
#include "../kernel/include/hal/hal_discovery.h"
#include "../kernel/include/hal/hal_mm.h"

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
  limits->dma32_end = (phys_addr_t)(uintptr_t)&g_mock_ram[(16 * PAGE_SIZE) - 1];
  limits->normal_start = (phys_addr_t)(uintptr_t)&g_mock_ram[16 * PAGE_SIZE];
  limits->normal_end = (phys_addr_t)(uintptr_t)&g_mock_ram[sizeof(g_mock_ram) - 1];
  limits->flags = 0;
}

memory_node_id_t numa_get_current_node(void) { return 0; }
kthread_t *sched_current_thread(void) { return NULL; }
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

  pmm_block_t block = {0};
  assert(pmm_alloc_pages(0, PMM_ZONE_ANY, PMM_ALLOC_NONE, &block) == 0);

  page_t *p = phys_to_page(block.phys_addr);
  assert(p != NULL);
  assert(p->ref_count == 1);

  assert(pmm_ref_get(block.phys_addr) == 0);
  assert(p->ref_count == 2);

  assert(pmm_ref_put(block.phys_addr) == 0);
  assert(p->ref_count == 1);
  assert(p->state == PMM_PAGE_STATE_ALLOCATED);

  assert(pmm_pin(block.phys_addr) == 0);
  assert(p->pin_count == 1);

  assert(pmm_ref_put(block.phys_addr) != 0);
  assert(p->ref_count == 1);
  assert(p->pin_count == 1);

  assert(pmm_ref_put(block.phys_addr) != 0);
  assert(p->ref_count == 1);
  assert(p->pin_count == 1);

  assert(pmm_unpin(block.phys_addr) == 0);
  assert(p->pin_count == 0);
  assert(pmm_unpin(block.phys_addr) != 0);
  assert(p->pin_count == 0);

  assert(pmm_ref_put(block.phys_addr) == 0);
  assert(p->state == PMM_PAGE_STATE_FREE);

  printf("test_pmm_refpin passed\n");
  return 0;
}
