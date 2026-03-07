#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/mm.h"

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}

void mm_free_page(phys_addr_t page) {
    (void)page;
}

int mm_pmm_init(void* memory_map, uint32_t map_size) {
    (void)memory_map;
    (void)map_size;
    return 0;
}

void mm_inc_page_ref(phys_addr_t page) { (void)page; }
void hal_tlb_flush(unsigned long long vaddr) { (void)vaddr; }

int main(void) {
    // vmm wrappers should reject invalid inputs in baseline implementation
    assert(vmm_map_page(0U, 0U, 0U) != 0);
    assert(vmm_unmap_page(0U) != 0);

    printf("Memory edge-case tests passed.\n");
    return 0;
}
