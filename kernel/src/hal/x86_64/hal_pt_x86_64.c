#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm.h"
#include "../../../include/numa.h"

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

#define X86_PT_PRESENT  (1ULL << 0)
#define X86_PT_RW       (1ULL << 1)
#define X86_PT_USER     (1ULL << 2)
#define X86_PT_PWT      (1ULL << 3)
#define X86_PT_PCD      (1ULL << 4)
#define X86_PT_ACCESSED (1ULL << 5)
#define X86_PT_DIRTY    (1ULL << 6)
#define X86_PT_HUGE     (1ULL << 7)
#define X86_PT_GLOBAL   (1ULL << 8)
#define X86_PT_NX       (1ULL << 63)

#define X86_PAGE_MASK   (~0xFFFULL)

typedef struct {
    uint64_t entries[512];
} pt_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & X86_PAGE_MASK;
}

static uint64_t flags_to_x86(uint32_t flags) {
    uint64_t pte_flags = X86_PT_PRESENT;

    if (flags & HAL_PT_FLAG_WRITE)   pte_flags |= X86_PT_RW;
    if (flags & HAL_PT_FLAG_USER)    pte_flags |= X86_PT_USER;
    if (!(flags & HAL_PT_FLAG_EXEC)) pte_flags |= X86_PT_NX;
    if (flags & HAL_PT_FLAG_NOCACHE) pte_flags |= X86_PT_PCD;
    if (flags & HAL_PT_FLAG_DEVICE)  pte_flags |= (X86_PT_PCD | X86_PT_PWT);
    if (flags & HAL_PT_FLAG_GLOBAL)  pte_flags |= X86_PT_GLOBAL;
    // SW flag COW can be mapped to a custom bit if needed, but not hardware interpreted.

    return pte_flags;
}

static uint32_t x86_to_flags(uint64_t pte_flags) {
    uint32_t flags = HAL_PT_FLAG_READ;

    if (pte_flags & X86_PT_RW)       flags |= HAL_PT_FLAG_WRITE;
    if (pte_flags & X86_PT_USER)     flags |= HAL_PT_FLAG_USER;
    if (!(pte_flags & X86_PT_NX))    flags |= HAL_PT_FLAG_EXEC;
    if (pte_flags & X86_PT_PCD)      flags |= HAL_PT_FLAG_NOCACHE;
    if ((pte_flags & X86_PT_PCD) && (pte_flags & X86_PT_PWT)) flags |= HAL_PT_FLAG_DEVICE;
    if (pte_flags & X86_PT_GLOBAL)   flags |= HAL_PT_FLAG_GLOBAL;

    return flags;
}


phys_addr_t x86_pt_create_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pt_t* pml4 = (pt_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    phys_addr_t kernel_root = kernel_root_table;
    if (kernel_root != 0U) {
        pt_t* kernel_pml4 = (pt_t*)P2V(kernel_root);
        // Link kernel space: Map the top half
        // A minimal implementation may just copy entry 511, or 256-511
        for (int i = 256; i < 512; i++) {
             pml4->entries[i] = kernel_pml4->entries[i];
        }
    }

    return root;
}

static void x86_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 1) {
        pt_t* pt = (pt_t*)P2V(table);
        // User space is 0-255 in PML4
        int max_idx = (level == 4) ? 256 : 512;
        for (int i = 0; i < max_idx; i++) {
            if (pt->entries[i] & X86_PT_PRESENT) {
                if ((level == 3 || level == 2) && (pt->entries[i] & X86_PT_HUGE)) {
                    continue; // Huge page, don't recurse down
                }
                x86_pt_destroy_recursive(pt->entries[i] & X86_PAGE_MASK, level - 1);
            }
        }
    }
    mm_free_page(table);
}

void x86_pt_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        x86_pt_destroy_recursive(root_pt, 4);
    }
}

int x86_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)P2V(root_pt);

    // Provide generic permissive access (RW | User) to intermediate tables
    // The leaf PTE restricts the final access permissions.
    uint64_t dir_flags = X86_PT_PRESENT | X86_PT_RW | X86_PT_USER;

    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pdp = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pdp) return -2;
        pt_t* pdp_ptr = (pt_t*)P2V(new_pdp);
        for(int i=0; i<512; i++) pdp_ptr->entries[i] = 0;
        pml4->entries[pml4_idx] = new_pdp | dir_flags;
    }

    pt_t* pdp = (pt_t*)P2V(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pd) return -2;
        pt_t* pd_ptr = (pt_t*)P2V(new_pd);
        for(int i=0; i<512; i++) pd_ptr->entries[i] = 0;
        pdp->entries[pdp_idx] = new_pd | dir_flags;
    }

    pt_t* pd = (pt_t*)P2V(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pt = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pt) return -2;
        pt_t* pt_ptr = (pt_t*)P2V(new_pt);
        for(int i=0; i<512; i++) pt_ptr->entries[i] = 0;
        pd->entries[pd_idx] = new_pt | dir_flags;
    }

    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & X86_PAGE_MASK);

    uint64_t pte_flags = flags_to_x86(flags);
    pt->entries[pt_idx] = aligned_paddr | pte_flags;

    return 0;
}

int x86_pt_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)P2V(root_pt);
    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pdp = (pt_t*)P2V(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pd = (pt_t*)P2V(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & X86_PAGE_MASK);

    if ((pt->entries[pt_idx] & X86_PT_PRESENT) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = pt->entries[pt_idx] & X86_PAGE_MASK;
    }

    pt->entries[pt_idx] = 0;

    return 0;
}

int x86_pt_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)P2V(root_pt);
    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pdp = (pt_t*)P2V(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pd = (pt_t*)P2V(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & X86_PAGE_MASK);

    if ((pt->entries[pt_idx] & X86_PT_PRESENT) == 0) return -2;

    uint64_t paddr = pt->entries[pt_idx] & X86_PAGE_MASK;
    uint64_t pte_flags = flags_to_x86(new_flags);

    pt->entries[pt_idx] = paddr | pte_flags;

    return 0;
}

int x86_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)P2V(root_pt);
    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pdp = (pt_t*)P2V(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pd = (pt_t*)P2V(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & X86_PAGE_MASK);

    if ((pt->entries[pt_idx] & X86_PT_PRESENT) == 0) return -2;

    if (paddr) *paddr = pt->entries[pt_idx] & X86_PAGE_MASK;
    if (flags) *flags = x86_to_flags(pt->entries[pt_idx] & ~X86_PAGE_MASK);

    return 0;
}

static inline void write_cr4(uint64_t val) {
    asm volatile("mov %0, %%cr4" :: "r"(val) : "memory");
}

static inline uint64_t read_cr4(void) {
    uint64_t val;
    asm volatile("mov %%cr4, %0" : "=r"(val));
    return val;
}

void x86_64_init_hardening(void) {
    uint64_t cr4 = read_cr4();
    // Enable SMEP (Supervisor Mode Execution Prevention)
    // Bit 20 in CR4
    cr4 |= (1ULL << 20);
    // Enable SMAP (Supervisor Mode Access Prevention)
    // Bit 21 in CR4
    cr4 |= (1ULL << 21);
    write_cr4(cr4);
}

hal_pt_ops_t x86_hal_pt_ops = {
    .create_address_space  = x86_pt_create_address_space,
    .destroy_address_space = x86_pt_destroy_address_space,
    .map_page              = x86_pt_map_page,
    .unmap_page            = x86_pt_unmap_page,
    .protect_page          = x86_pt_protect_page,
    .query_page            = x86_pt_query_page,
};

// --- x86_64 TLB Operations ---

extern bool g_x86_pcid_supported;

static void x86_tlb_flush_page_local(virt_addr_t vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

static void x86_tlb_flush_all_local(void) {
    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    if (g_x86_pcid_supported) {
        cr3 &= ~(1ULL << 63); // Clear bit 63 to flush
    }
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

static void x86_tlb_flush_asid_local(uint16_t asid) {
    if (g_x86_pcid_supported) {
        struct {
            uint64_t pcid: 12;
            uint64_t reserved: 52;
        } desc = { .pcid = asid, .reserved = 0 };
        __asm__ volatile("invpcid %0, %1" :: "m"(desc), "r"(1ULL) : "memory");
    } else {
        x86_tlb_flush_all_local();
    }
}

static void x86_tlb_flush_page_remote(uint16_t target_core, uint16_t asid, virt_addr_t vaddr) {
    extern void mm_remote_tlb_flush(uint32_t target_core, uint64_t as_id, virt_addr_t va);
    mm_remote_tlb_flush(target_core, asid, vaddr);
}

static void x86_tlb_flush_all_remote(uint16_t target_core, uint16_t asid) {
    extern void mm_remote_tlb_flush(uint32_t target_core, uint64_t as_id, virt_addr_t va);
    mm_remote_tlb_flush(target_core, asid, 0); // 0 signifies all
}

static void x86_tlb_flush_page_broadcast(uint16_t asid, virt_addr_t vaddr) {
    extern void mm_remote_tlb_shootdown_mask(uint64_t core_membership_mask, uint64_t as_id, virt_addr_t va);
    // Broadcast to all other cores
    mm_remote_tlb_shootdown_mask(~0ULL, asid, vaddr);
}

static void x86_tlb_flush_all_broadcast(uint16_t asid) {
    extern void mm_remote_tlb_shootdown_mask(uint64_t core_membership_mask, uint64_t as_id, virt_addr_t va);
    mm_remote_tlb_shootdown_mask(~0ULL, asid, 0);
}

hal_tlb_ops_t x86_hal_tlb_ops = {
    .flush_page_local      = x86_tlb_flush_page_local,
    .flush_all_local       = x86_tlb_flush_all_local,
    .flush_asid_local      = x86_tlb_flush_asid_local,
    .flush_page_remote     = x86_tlb_flush_page_remote,
    .flush_all_remote      = x86_tlb_flush_all_remote,
    .flush_page_broadcast  = x86_tlb_flush_page_broadcast,
    .flush_all_broadcast   = x86_tlb_flush_all_broadcast,
};
