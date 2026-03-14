#include "../../../include/hal/vmm.h"
#include "../../../include/hal/mmu_ops.h"
#include "../../../include/numa.h"

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

#define VMM_FLAG_PRESENT 0x1U

// Use a reserved bit in the 32-bit flags return to pass down NX to map/update
#define X86_FLAG_NX (1U << 31)

typedef struct {
    uint64_t entries[512];
} pt_t, pd_t, pdp_t, pml4_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & ~(virt_addr_t)(PAGE_SIZE - 1U);
}

phys_addr_t hal_vmm_init_root(void) {
    phys_addr_t root_dir_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (root_dir_phys == 0U) {
        return 0;
    }

    pml4_t* pml4 = (pml4_t*)P2V(root_dir_phys);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    return root_dir_phys;
}

int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U || paddr == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) {
        phys_addr_t new_pdp = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pdp) return -2;
        pdp_t* pdp_ptr = (pdp_t*)P2V(new_pdp);
        for(int i=0; i<512; i++) pdp_ptr->entries[i] = 0;
        pml4->entries[pml4_idx] = new_pdp | VMM_FLAG_PRESENT | (flags & PAGE_USER) | CAP_RIGHT_WRITE;
    }

    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) {
        phys_addr_t new_pd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pd) return -2;
        pd_t* pd_ptr = (pd_t*)P2V(new_pd);
        for(int i=0; i<512; i++) pd_ptr->entries[i] = 0;
        pdp->entries[pdp_idx] = new_pd | VMM_FLAG_PRESENT | (flags & PAGE_USER) | CAP_RIGHT_WRITE;
    }

    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) {
        phys_addr_t new_pt = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pt) return -2;
        pt_t* pt_ptr = (pt_t*)P2V(new_pt);
        for(int i=0; i<512; i++) pt_ptr->entries[i] = 0;
        pd->entries[pd_idx] = new_pt | VMM_FLAG_PRESENT | (flags & PAGE_USER) | CAP_RIGHT_WRITE;
    }

    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    uint64_t final_flags = flags & ~X86_FLAG_NX;
    if (flags & X86_FLAG_NX) {
        final_flags |= (1ULL << 63);
    }
    pt->entries[pt_idx] = aligned_paddr | VMM_FLAG_PRESENT | final_flags;

    return 0;
}

int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr) {
    if (root_table == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    if (unmapped_paddr) {
        *unmapped_paddr = pt->entries[pt_idx] & ~0xFFFULL;
    }

    pt->entries[pt_idx] = 0;
    return 0;
}

phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pml4_t* pml4 = (pml4_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    if (kernel_root_table != 0U) {
        pml4_t* kernel_pml4 = (pml4_t*)P2V(kernel_root_table);
        pml4->entries[511] = kernel_pml4->entries[511];
    }

    return root;
}

int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    if (root_table == 0U) return -1;

    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    uint64_t old_entry = pt->entries[pt_idx];
    if ((old_entry & VMM_FLAG_PRESENT) == 0) return -2;

    if (paddr) *paddr = old_entry & ~0xFFFULL;
    if (flags) *flags = old_entry & 0xFFFULL;

    return 0;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    uint64_t final_flags = flags & ~X86_FLAG_NX;
    if (flags & X86_FLAG_NX) {
        final_flags |= (1ULL << 63);
    }
    pt->entries[pt_idx] = aligned_paddr | VMM_FLAG_PRESENT | final_flags;
    return 0;
}

// -----------------------------------------------------------------------------
// mmu_ops_t Wrapper implementations
// -----------------------------------------------------------------------------

static phys_addr_t x86_mmu_create_table(void) {
    return hal_vmm_init_root();
}

static void x86_mmu_destroy_table_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 1) {
        pt_t* pt = (pt_t*)P2V(table);
        for (int i = 0; i < (level == 4 ? 256 : 512); i++) { // Skip kernel half on pml4
            if (pt->entries[i] & VMM_FLAG_PRESENT) {
                if ((level == 3 || level == 2) && (pt->entries[i] & (1ULL << 7))) {
                    // Huge page, don't recurse
                    continue;
                }
                x86_mmu_destroy_table_recursive(pt->entries[i] & ~0xFFFULL, level - 1);
            }
        }
    }
    mm_free_page(table);
}

static void x86_mmu_destroy_table(phys_addr_t root) {
    if (root) {
        x86_mmu_destroy_table_recursive(root, 4);
    }
}

static phys_addr_t x86_mmu_clone_kernel(phys_addr_t kernel_root) {
    return hal_vmm_setup_address_space(kernel_root);
}

// Translate generic mmu_flags_t to x86-specific VMM flags
static uint32_t flags_to_x86(mmu_flags_t f) {
    uint32_t pte = 0;
    if (f & MMU_WRITE)   pte |= CAP_RIGHT_WRITE;
    if (f & MMU_USER)    pte |= PAGE_USER;
    if (f & MMU_COW)     pte |= PAGE_COW;
    if (!(f & MMU_EXEC)) pte |= X86_FLAG_NX; // Pass NX down via reserved bit
    if (f & MMU_NOCACHE) pte |= (1ULL << 4);  // PCD bit
    if (f & MMU_DEVICE)  pte |= (1ULL << 4) | (1ULL << 3); // PCD | PWT
    return pte;
}

// Translate x86-specific VMM flags to generic mmu_flags_t
static mmu_flags_t x86_to_flags(uint32_t pte) {
    mmu_flags_t f = MMU_READ;
    if (pte & CAP_RIGHT_WRITE) f |= MMU_WRITE;
    if (pte & PAGE_USER)       f |= MMU_USER;
    if (pte & PAGE_COW)        f |= MMU_COW;
    return f;
}

static int x86_mmu_map(phys_addr_t root, virt_addr_t virt, phys_addr_t phys,
                       size_t size, mmu_flags_t flags) {
    // x86 HAL map operates on single pages. We iterate over the size.
    size_t mapped = 0;
    while (mapped < size) {
        int ret = hal_vmm_map_page(root, virt + mapped, phys + mapped, flags_to_x86(flags));
        if (ret < 0) return ret;
        mapped += PAGE_SIZE;
    }
    return 0;
}

static int x86_mmu_unmap(phys_addr_t root, virt_addr_t virt, size_t size, phys_addr_t *unmapped_phys) {
    size_t unmapped = 0;
    phys_addr_t first_phys = 0;

    while (unmapped < size) {
        phys_addr_t phys = 0;
        int ret = hal_vmm_unmap_page(root, virt + unmapped, &phys);
        if (ret < 0) return ret;
        if (unmapped == 0) first_phys = phys;
        unmapped += PAGE_SIZE;
    }

    if (unmapped_phys) {
        *unmapped_phys = first_phys;
    }
    return 0;
}

static int x86_mmu_protect(phys_addr_t root, virt_addr_t virt, size_t size, mmu_flags_t new_flags) {
    size_t updated = 0;
    while (updated < size) {
        phys_addr_t phys = 0;
        uint32_t old_flags = 0;
        int ret = hal_vmm_get_mapping(root, virt + updated, &phys, &old_flags);
        if (ret == 0 && phys != 0) {
            hal_vmm_update_mapping(root, virt + updated, phys, flags_to_x86(new_flags));
        }
        updated += PAGE_SIZE;
    }
    return 0;
}

static int x86_mmu_query(phys_addr_t root, virt_addr_t virt, phys_addr_t *phys_out, mmu_flags_t *flags_out) {
    uint32_t x86_flags = 0;
    int ret = hal_vmm_get_mapping(root, virt, phys_out, &x86_flags);
    if (ret == 0 && flags_out) {
        *flags_out = x86_to_flags(x86_flags);
    }
    return ret;
}

static void x86_mmu_activate(phys_addr_t root) {
    __asm__ volatile("mov %0, %%cr3" :: "r"((uintptr_t)root) : "memory");
}

static void x86_mmu_deactivate(void) {
    // No-op or switch to kernel root
}

static void x86_mmu_tlb_flush_page(virt_addr_t virt) {
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

static void x86_mmu_tlb_flush_all(void) {
    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

static void x86_mmu_tlb_flush_asid(uint16_t asid) {
    (void)asid; // Not using PCID in baseline right now
}

static size_t x86_huge_pages[] = {0x200000, 0x40000000, 0}; // 2MB, 1GB

mmu_ops_t x86_64_mmu_ops = {
    .create_table     = x86_mmu_create_table,
    .destroy_table    = x86_mmu_destroy_table,
    .clone_kernel     = x86_mmu_clone_kernel,
    .map              = x86_mmu_map,
    .unmap            = x86_mmu_unmap,
    .protect          = x86_mmu_protect,
    .query            = x86_mmu_query,
    .activate         = x86_mmu_activate,
    .deactivate       = x86_mmu_deactivate,
    .tlb_flush_page   = x86_mmu_tlb_flush_page,
    .tlb_flush_all    = x86_mmu_tlb_flush_all,
    .tlb_flush_asid   = x86_mmu_tlb_flush_asid,

    .page_size        = 4096,
    .huge_page_sizes  = x86_huge_pages,
    .levels           = 4,
    .has_nx           = true,
    .asid_bits        = 0, // Not using PCID initially
    .has_user_kernel_split = false, // x86 usually uses a shared address space with user/supervisor bits
};

void x86_mmu_detect(mmu_ops_t *ops) {
    // Stub for CPUID detection (e.g., 1GB pages, LA57)
    // Could read CPUID to modify ops->levels or ops->huge_page_sizes
    (void)ops;
}
