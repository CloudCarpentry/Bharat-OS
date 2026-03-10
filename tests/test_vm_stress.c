#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/mm.h"
#include "../kernel/include/hal/mmu_ops.h"

#define MAX_PHYS_PAGES 100

typedef struct {
    phys_addr_t paddr;
    int is_allocated;
    int ref_count;
    virt_addr_t last_vaddr;
} mock_page_t;

typedef struct {
    virt_addr_t virt;
    phys_addr_t phys;
    int mapped;
} mock_pte_t;

static mock_pte_t g_page_table[MAX_PHYS_PAGES];

static int mock_mmu_map(phys_addr_t root, virt_addr_t virt, phys_addr_t phys, size_t size, mmu_flags_t flags) {
    (void)root; (void)size; (void)flags;
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (!g_page_table[i].mapped) {
            g_page_table[i].virt = virt;
            g_page_table[i].phys = phys;
            g_page_table[i].mapped = 1;
            return 0;
        }
    }
    return -1;
}

static int mock_mmu_unmap(phys_addr_t root, virt_addr_t virt, size_t size, phys_addr_t *unmapped_phys) {
    (void)root; (void)size;
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (g_page_table[i].mapped && g_page_table[i].virt == virt) {
            if (unmapped_phys) *unmapped_phys = g_page_table[i].phys;
            g_page_table[i].mapped = 0;
            return 0;
        }
    }
    if (unmapped_phys) *unmapped_phys = 0;
    return -1;
}

static int g_tlb_flushes = 0;
static virt_addr_t g_last_tlb_vaddr = 0;

static int g_allocated_pages = 0;
static mock_page_t g_page_pool[MAX_PHYS_PAGES];

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    if (g_allocated_pages >= MAX_PHYS_PAGES) {
        return 0; // Simulate exhaustion
    }

    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (!g_page_pool[i].is_allocated) {
            g_page_pool[i].is_allocated = 1;
            g_page_pool[i].ref_count = 1;
            g_allocated_pages++;
            return g_page_pool[i].paddr;
        }
    }
    return 0;
}

void mm_free_page(phys_addr_t page) {
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (g_page_pool[i].paddr == page) {
            assert(g_page_pool[i].is_allocated == 1); // Catch double free
            g_page_pool[i].ref_count--;
            if (g_page_pool[i].ref_count == 0) {
                g_page_pool[i].is_allocated = 0;
                g_allocated_pages--;
            }
            return;
        }
    }
    assert(0); // Freeing a page not in pool
}

void mm_inc_page_ref(phys_addr_t page) {
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (g_page_pool[i].paddr == page && g_page_pool[i].is_allocated) {
            g_page_pool[i].ref_count++;
            return;
        }
    }
}

void hal_tlb_flush(unsigned long long vaddr) {
    g_tlb_flushes++;
    g_last_tlb_vaddr = (virt_addr_t)vaddr;
}

// Minimal stub for testing mapping failures
void tlb_shootdown(virt_addr_t vaddr);

int bharat_addr_token_validate(const bharat_addr_token_t* token, uint64_t request_paddr, uint64_t request_size, uint32_t request_perms) {
    (void)token; (void)request_paddr; (void)request_size; (void)request_perms;
    return 0;
}

#include <stdlib.h>

int main(void) {
    printf("Running VM stress and memory pressure tests...\n");

    // Override active_mmu
    extern mmu_ops_t *active_mmu;
    if (active_mmu) {
        active_mmu->map = mock_mmu_map;
        active_mmu->unmap = mock_mmu_unmap;
    }

    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        g_page_table[i].mapped = 0;
    }

    // Initialize pool
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        g_page_pool[i].paddr = (phys_addr_t)(i + 1) * 0x1000;
        g_page_pool[i].is_allocated = 0;
        g_page_pool[i].ref_count = 0;
    }
    g_allocated_pages = 0;

    assert(vmm_init() == 0);

    // Test mapping until exhaustion
    int mapped_count = 0;
    for (int i = 0; i < MAX_PHYS_PAGES + 10; i++) {
        phys_addr_t paddr = mm_alloc_page(0);
        if (paddr == 0) {
            break;
        }

        if (vmm_map_page((virt_addr_t)(i + 1) * 0x1000, paddr, PAGE_USER) == 0) {
            mapped_count++;
        }
    }

    assert(mapped_count == MAX_PHYS_PAGES);
    assert(g_allocated_pages == MAX_PHYS_PAGES);

    // Test unmap and tracking logic
    virt_addr_t unmap_vaddr = 5 * 0x1000;

    // vmm.c's `mm_vmm_unmap_page` uses `active_mmu->unmap`.
    // It will now correctly get the mapped phys address and call `mm_free_page()`.
    assert(vmm_unmap_page(unmap_vaddr) == 0);
    assert(g_allocated_pages == MAX_PHYS_PAGES - 1);

    // Clean up all to assert zero allocations left at teardown
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (g_page_table[i].mapped) {
             vmm_unmap_page(g_page_table[i].virt);
        }
    }

    // Any allocated pages that were not mapped but were requested must be manually freed
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (g_page_pool[i].is_allocated) {
            mm_free_page(g_page_pool[i].paddr);
        }
    }

    assert(g_allocated_pages == 0);

    // Stress Test with random map/unmap
    unsigned int seed = 123;
    for (int i = 0; i < 500; i++) {
        int action = rand_r(&seed) % 2;
        if (action == 0) {
            phys_addr_t p = mm_alloc_page(0);
            if (p) {
                virt_addr_t v = p + 0x1000;
                vmm_map_page(v, p, PAGE_USER);
            }
        } else {
            // Unmap random
            for (int k = 0; k < MAX_PHYS_PAGES; k++) {
                if (g_page_table[k].mapped) {
                    vmm_unmap_page(g_page_table[k].virt);
                    break;
                }
            }
        }
    }

    // Teardown
    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (g_page_table[i].mapped) {
             vmm_unmap_page(g_page_table[i].virt);
        }
    }

    for (int i = 0; i < MAX_PHYS_PAGES; i++) {
        if (g_page_pool[i].is_allocated) {
            mm_free_page(g_page_pool[i].paddr);
        }
    }
    assert(g_allocated_pages == 0);

    printf("VM stress tests passed successfully.\n");
    return 0;
}
