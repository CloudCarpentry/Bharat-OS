#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mm/pt_cache.h>
static unsigned allocations;
static int allocation_budget = -1;
phys_addr_t pt_cache_alloc(void) {
    if (allocation_budget == 0) return 0;
    if (allocation_budget > 0) --allocation_budget;
    void *p = aligned_alloc(4096, 4096);
    if (!p) return 0;
    memset(p, 0, 4096); ++allocations;
    return (uintptr_t)p;
}
void pt_cache_free(phys_addr_t p) { free((void *)(uintptr_t)p); --allocations; }
void *physmap_phys_to_virt(phys_addr_t p) { return (void *)(uintptr_t)p; }
#include "../../../core/arch/x86/x86_64/hal_pt_x86_64.c"
static int test_map(phys_addr_t root, virt_addr_t va, phys_addr_t pa, uint32_t flags) {
    page_table_walk_result_t result;
    int status = x86_pt_walk(root, va, true, flags, &result);
    if (status == 0 && result.entry_va) *(uint64_t *)result.entry_va = pa | flags_to_x86(flags);
    return status;
}
int main(void) {
    phys_addr_t kernel = x86_pt_create_address_space(0);
    assert(kernel);
    assert(test_map(kernel, 0x100000, 0x100000, HAL_PT_FLAG_EXEC) == 0);
    unsigned baseline = allocations;
    for (allocation_budget = 0; allocation_budget < 4;) {
        int budget = allocation_budget;
        assert(x86_pt_create_address_space(kernel) == 0);
        assert(allocations == baseline);
        allocation_budget = budget + 1;
    }
    allocation_budget = -1;
    phys_addr_t a = x86_pt_create_address_space(kernel);
    phys_addr_t b = x86_pt_create_address_space(kernel);
    assert(a && b);
    assert(test_map(a, 0x40000000, 0x20000000, HAL_PT_FLAG_USER | HAL_PT_FLAG_EXEC) == 0);
    assert(test_map(b, 0x40000000, 0x30000000, HAL_PT_FLAG_USER) == 0);
    phys_addr_t mapped; uint32_t flags;
    assert(x86_pt_query_page(a, 0x40000000, &mapped, &flags) == 0);
    assert(mapped == 0x20000000 && (flags & HAL_PT_FLAG_EXEC));
    assert(x86_pt_query_page(b, 0x40000000, &mapped, &flags) == 0);
    assert(mapped == 0x30000000 && !(flags & HAL_PT_FLAG_EXEC));
    assert(x86_pt_query_page(kernel, 0x40000000, &mapped, &flags) != 0);
    phys_addr_t c = x86_pt_create_address_space(a);
    assert(c);
    assert(x86_pt_query_page(c, 0x40000000, &mapped, &flags) != 0);
    assert(x86_pt_query_page(c, 0x100000, &mapped, &flags) == 0 && mapped == 0x100000);
    x86_pt_destroy_address_space(a); x86_pt_destroy_address_space(b);
    x86_pt_destroy_address_space(c); x86_pt_destroy_address_space(kernel);
    assert(allocations == 0);
    puts("PASS: private process mappings, supervisor inheritance, and table cleanup");
}
