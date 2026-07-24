#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm.h"

extern int x86_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags);
extern int x86_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags);
extern int x86_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags);
extern int x86_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size);
extern phys_addr_t x86_pt_create_address_space(phys_addr_t kernel_root_table);
extern void x86_pt_destroy_address_space(phys_addr_t root_pt);

extern bool g_x86_pcid_supported;

#define TEST_POOL_PAGES 8192
static unsigned char g_pool[TEST_POOL_PAGES][PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t g_used[TEST_POOL_PAGES];
static size_t g_allocs;
static size_t g_frees;

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    for (size_t i = 0; i < TEST_POOL_PAGES; i++) {
        if (!g_used[i]) {
            g_used[i] = 1;
            memset(g_pool[i], 0, PAGE_SIZE);
            g_allocs++;
            return (phys_addr_t)(uintptr_t)&g_pool[i][0];
        }
    }
    return 0;
}

void mm_free_page(phys_addr_t page) {
    uintptr_t addr = (uintptr_t)page;
    for (size_t i = 0; i < TEST_POOL_PAGES; i++) {
        if ((uintptr_t)&g_pool[i][0] == addr) {
            assert(g_used[i]);
            g_used[i] = 0;
            g_frees++;
            return;
        }
    }
    assert(!"free of unknown page");
}

int main(void) {
    phys_addr_t as = x86_pt_create_address_space(0);
    assert(as != 0);

    const virt_addr_t va = 0x40000000ULL;
    const phys_addr_t pa = 0x20000000ULL;
    const size_t huge = 2 * 1024 * 1024ULL;

    assert(x86_pt_map_range(as, va, pa, huge,
            HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_USER | HAL_PT_FLAG_EXEC | HAL_PT_FLAG_LARGE_2M) == 0);

    phys_addr_t qpa = 0;
    uint32_t qf = 0;
    assert(x86_pt_query_page(as, va + 0x1000, &qpa, &qf) == 0);
    assert(qpa == pa + 0x1000);
    assert(qf & HAL_PT_FLAG_LARGE_2M);
    assert(qf & HAL_PT_FLAG_EXEC);
    assert(qf & HAL_PT_FLAG_USER);

    // Split huge page by remapping one 4 KiB page with NX supervisor-only attributes.
    assert(x86_pt_map_page(as, va + 0x3000, pa + 0x3000, HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE) == 0);
    assert(x86_pt_query_page(as, va + 0x3000, &qpa, &qf) == 0);
    assert((qf & HAL_PT_FLAG_LARGE_2M) == 0);
    assert((qf & HAL_PT_FLAG_USER) == 0);

    assert(x86_pt_unmap_range(as, va, huge) == 0);
    x86_pt_destroy_address_space(as);

    assert(g_allocs == g_frees);
    printf("pt_x86_64_tests: PASS\n");
    return 0;
}
