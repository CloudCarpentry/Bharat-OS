#include "../../../include/mm.h"
#include "../../../include/spinlock.h"

// Stub for a dedicated page-table page pool.
// In a full implementation, this avoids going to the buddy allocator for standard page table allocations,
// keeping them hot in cache and potentially pre-zeroed.

#define PT_POOL_SIZE 256
static phys_addr_t g_pt_pool[PT_POOL_SIZE];
static int g_pt_pool_count = 0;
static spinlock_t g_pt_pool_lock = {0};

phys_addr_t mm_alloc_pt_page(void) {
    spin_lock(&g_pt_pool_lock);
    if (g_pt_pool_count > 0) {
        phys_addr_t p = g_pt_pool[--g_pt_pool_count];
        spin_unlock(&g_pt_pool_lock);
        return p;
    }
    spin_unlock(&g_pt_pool_lock);
    return mm_alloc_page(0); // fallback
}

void mm_free_pt_page(phys_addr_t paddr) {
    spin_lock(&g_pt_pool_lock);
    if (g_pt_pool_count < PT_POOL_SIZE) {
        g_pt_pool[g_pt_pool_count++] = paddr;
        spin_unlock(&g_pt_pool_lock);
        return;
    }
    spin_unlock(&g_pt_pool_lock);
    mm_free_page(paddr); // fallback
}

// Hugepage promotion stub
// Periodically or on fault, attempts to merge 512 4K pages into a 2MB page
void mm_promote_hugepage(address_space_t *as, virt_addr_t vaddr) {
    // Requires physical contiguity check, permission consistency, and PT remap
    // To be implemented fully in future passes
    (void)as;
    (void)vaddr;
}
