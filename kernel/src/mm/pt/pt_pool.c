#include "../../include/mm.h"
#include "../../include/spinlock.h"
#include "../../include/slab.h"
#include "../../include/mm/pt_cache.h"
#include "../../include/mm/physmap.h"

// Dedicated page-table page pool/cache.
// We maintain a direct pool of pre-allocated physical pages to avoid
// relying purely on slab caches that might not provide strict page alignment
// or phys-mapped safety guarantees easily during early boot.

#define PT_POOL_SIZE 256
static phys_addr_t g_pt_pool[PT_POOL_SIZE];
static int g_pt_pool_count = 0;
static spinlock_t g_pt_pool_lock = {0};

static kcache_t* g_pt_cache = NULL;

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

void pt_cache_init(void) {
    // For now, use the dedicated mm_alloc_pt_page block pool
    // A true SLAB/kcache approach for page tables is tricky because they MUST be page-aligned.
    g_pt_cache = kcache_create("pt_cache", PAGE_SIZE);
}

phys_addr_t pt_cache_alloc(void) {
    pmm_block_t block;
    // We now use the hardened PMM allocation with zeroing natively instead of custom pool fallback
    // We request a single page (order 0) that is explicitly zeroed.
    if (pmm_alloc_pages(0, PMM_ZONE_ANY, PMM_ALLOC_ZERO, &block) == 0) {
        page_t *p = phys_to_page(block.phys_addr);
        if (p) p->owner_class = PMM_OWNER_CLASS_PAGETABLE;
        return block.phys_addr;
    }
    return 0;
}

void pt_cache_free(phys_addr_t pa) {
    pmm_block_t block;
    block.phys_addr = pa;
    block.page_count = 1;
    block.order = 0;
    pmm_free_pages(&block);
}

// Hugepage promotion stub
// Periodically or on fault, attempts to merge 512 4K pages into a 2MB page
void mm_promote_hugepage(address_space_t *as, virt_addr_t vaddr) {
    // Requires physical contiguity check, permission consistency, and PT remap
    // To be implemented fully in future passes
    (void)as;
    (void)vaddr;
}
