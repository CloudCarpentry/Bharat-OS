#include "../../kernel/include/hal/hal_mpa.h"
#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_pt_walk.h"
#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm.h"
#include "../../kernel/include/numa.h"
#include "../../kernel/include/mm/physmap.h"
#include "../../kernel/include/mm/pt_cache.h"
#include "../../kernel/include/arch/arch_cpu_caps.h"
#include "../../kernel/include/arch/arch_tlb_accel.h"
#include <stdbool.h>

// Direct-Map Subsystem Configuration
// For x86_64, the standard high-half mapping base
const virt_addr_t g_kernel_virt_offset = 0xFFFF800000000000ULL;
const size_t g_kernel_physmap_size = 0x8000000000ULL; // 512GB

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

#define X86_MAX_PHYS_ADDR_BITS 52U
#define X86_PAGE_MASK   ((((1ULL << X86_MAX_PHYS_ADDR_BITS) - 1ULL)) & ~0xFFFULL)
#define X86_LARGE_2M_SIZE (1ULL << 21)

static void x86_tlb_flush_page_local(virt_addr_t vaddr);
bool g_x86_mmu_finalized = false;
extern const virt_addr_t g_kernel_virt_offset;

static void* x86_phys_to_virt(phys_addr_t phys) { 
    if (!g_x86_mmu_finalized) {
        return (void*)phys;
    }
    return (void*)(phys + g_kernel_virt_offset); 
}

// Arch-private raw descriptor
typedef uint64_t pte_raw_t;

typedef struct {
    pte_raw_t entries[512];
} pt_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & X86_PAGE_MASK;
}

static phys_addr_t pte_to_pa(uint64_t pte, uint8_t page_shift) {
    uint64_t page_mask = ~((1ULL << page_shift) - 1ULL);
    return (phys_addr_t)(pte & X86_PAGE_MASK & page_mask);
}

static bool huge_2m_aligned(virt_addr_t va, phys_addr_t pa) {
    return ((va | pa) & (X86_LARGE_2M_SIZE - 1ULL)) == 0ULL;
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


static phys_addr_t x86_pt_create_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = pt_cache_alloc();
    if (root == 0U) {
        return 0;
    }
    
    // CRITICAL: Use identity mapping (just cast to pointer) to access page tables
    // instead of physmap_phys_to_virt(), which uses high canonical addresses.
    // High canonical mappings may be incomplete in newly created page tables.
    pt_t* pml4 = (pt_t*)(uintptr_t)root;
    
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    phys_addr_t kernel_root = kernel_root_table;
    if (kernel_root != 0U) {
        pt_t* kernel_pml4 = (pt_t*)physmap_phys_to_virt(kernel_root);

        // Link identity mapping (used during boot tests and initial kernel load)
        pml4->entries[0] = kernel_pml4->entries[0];

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
        pt_t* pt = (pt_t*)physmap_phys_to_virt(table);
        // User space is 0-255 in PML4, but skip 0 as it holds the identity map
        int max_idx = (level == 4) ? 256 : 512;
        for (int i = 0; i < max_idx; i++) {
            if (level == 4 && i == 0) {
                continue; // Skip identity mapping
            }
            if (pt->entries[i] & X86_PT_PRESENT) {
                if ((level == 3 || level == 2) && (pt->entries[i] & X86_PT_HUGE)) {
                    continue; // Huge page, don't recurse down
                }
                x86_pt_destroy_recursive(pt->entries[i] & X86_PAGE_MASK, level - 1);
            }
        }
    }
    pt_cache_free(table);
}

static void x86_pt_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        x86_pt_destroy_recursive(root_pt, 4);
    }
}

static int x86_pt_walk(phys_addr_t root_pt, virt_addr_t vaddr, bool create, uint32_t alloc_flags, page_table_walk_result_t *out_result) {
    if (root_pt == 0U || !out_result) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)x86_phys_to_virt(root_pt);
    uint64_t dir_flags = X86_PT_PRESENT | X86_PT_RW;
    if (alloc_flags & HAL_PT_FLAG_USER) {
        dir_flags |= X86_PT_USER;
    }

    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) {
        if (!create) {
            out_result->present = false;
            out_result->level = 4;
            return 0;
        }
        phys_addr_t new_pdp = pt_cache_alloc();
        if (!new_pdp) return -2;
        pt_t* pdp_ptr = (pt_t*)x86_phys_to_virt(new_pdp);
        for(int i=0; i<512; i++) pdp_ptr->entries[i] = 0;
        pml4->entries[pml4_idx] = new_pdp | dir_flags;
    }

    phys_addr_t pdp_pa = pml4->entries[pml4_idx] & X86_PAGE_MASK;
    pt_t* pdp = (pt_t*)x86_phys_to_virt(pdp_pa);

    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) {
        if (!create) {
            out_result->present = false;
            out_result->level = 3;
            return 0;
        }
        phys_addr_t new_pd = pt_cache_alloc();
        if (!new_pd) return -2;
        pt_t* pd_ptr = (pt_t*)x86_phys_to_virt(new_pd);
        for(int i=0; i<512; i++) pd_ptr->entries[i] = 0;
        pdp->entries[pdp_idx] = new_pd | dir_flags;
    } else if (pdp->entries[pdp_idx] & X86_PT_HUGE) {
        // 1GB huge page (not fully supported by map_4k, just report)
        out_result->present = true;
        out_result->level = 3;
        out_result->is_large = true;
        out_result->entry_pa = pdp_pa + pdp_idx * sizeof(pte_raw_t);
        out_result->entry_va = &pdp->entries[pdp_idx];
        out_result->raw_value = pdp->entries[pdp_idx];
        out_result->mapped_pa = pte_to_pa(pdp->entries[pdp_idx], 30);
        out_result->flags = x86_to_flags(pdp->entries[pdp_idx] & ~X86_PAGE_MASK);
        return 0;
    }

    phys_addr_t pd_pa = pdp->entries[pdp_idx] & X86_PAGE_MASK;
    pt_t* pd = (pt_t*)x86_phys_to_virt(pd_pa);

    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) {
        if (!create) {
            out_result->present = false;
            out_result->level = 2;
            return 0;
        }
        phys_addr_t new_pt = pt_cache_alloc();
        if (!new_pt) return -2;
        pt_t* pt_ptr = (pt_t*)x86_phys_to_virt(new_pt);
        for(int i=0; i<512; i++) pt_ptr->entries[i] = 0;
        pd->entries[pd_idx] = new_pt | dir_flags;
    } else if (pd->entries[pd_idx] & X86_PT_HUGE) {
        // 2MB huge page
        if (create && !(alloc_flags & HAL_PT_FLAG_LARGE_2M)) {
            // Split it if we need a 4K mapping here
            phys_addr_t huge_base = pte_to_pa(pd->entries[pd_idx], 21);
            phys_addr_t new_pt = pt_cache_alloc();
            if (!new_pt) return -2;
            pt_t* split_pt = (pt_t*)x86_phys_to_virt(new_pt);
            uint64_t split_flags = (pd->entries[pd_idx] & ~X86_PAGE_MASK) & ~X86_PT_HUGE;
            for (size_t i = 0; i < 512; i++) {
                split_pt->entries[i] = (huge_base + (i * PAGE_SIZE)) | split_flags;
            }
            pd->entries[pd_idx] = new_pt | dir_flags;
        } else {
            out_result->present = true;
            out_result->level = 2;
            out_result->is_large = true;
            out_result->entry_pa = pd_pa + pd_idx * sizeof(pte_raw_t);
            out_result->entry_va = &pd->entries[pd_idx];
            out_result->raw_value = pd->entries[pd_idx];
            out_result->mapped_pa = pte_to_pa(pd->entries[pd_idx], 21);
            out_result->flags = x86_to_flags(pd->entries[pd_idx] & ~X86_PAGE_MASK) | HAL_PT_FLAG_LARGE_2M;
            return 0;
        }
    }

    phys_addr_t pt_pa = pd->entries[pd_idx] & X86_PAGE_MASK;
    pt_t* pt = (pt_t*)x86_phys_to_virt(pt_pa);

    out_result->present = (pt->entries[pt_idx] & X86_PT_PRESENT) != 0;
    out_result->level = 1;
    out_result->is_large = false;
    out_result->entry_pa = pt_pa + pt_idx * sizeof(pte_raw_t);
    out_result->entry_va = &pt->entries[pt_idx];
    out_result->raw_value = pt->entries[pt_idx];
    out_result->mapped_pa = pte_to_pa(pt->entries[pt_idx], 12);
    out_result->flags = out_result->present ? x86_to_flags(pt->entries[pt_idx] & ~X86_PAGE_MASK) : 0;

    return 0;
}

const pt_walk_ops_t x86_pt_walk_ops = {
    .walk = x86_pt_walk,
    .update = NULL,
    .clear = NULL
};

const pt_walk_ops_t *active_pt_walk_ops = &x86_pt_walk_ops;

static int x86_pt_map_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    page_table_walk_result_t res;
    int rc = x86_pt_walk(root_pt, vaddr, true, flags, &res);
    if (rc != 0) return rc;

    if (res.level == 1 && res.entry_va) {
        pte_raw_t* pte = (pte_raw_t*)res.entry_va;
        *pte = aligned_paddr | flags_to_x86(flags);
        x86_tlb_flush_page_local(vaddr);
        return 0;
    }
    return -1; // Should not reach here for 4k map unless error
}

static int x86_pt_unmap_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    page_table_walk_result_t res;
    int rc = x86_pt_walk(root_pt, vaddr, false, 0, &res);
    if (rc != 0) return rc;

    if (!res.present) return -2;

    if (unmapped_paddr) {
        if (res.is_large && res.level == 2) {
            uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;
            *unmapped_paddr = res.mapped_pa + (pt_idx * PAGE_SIZE);
        } else {
            *unmapped_paddr = res.mapped_pa;
        }
    }

    if (res.entry_va) {
        pte_raw_t* pte = (pte_raw_t*)res.entry_va;
        *pte = 0;

        // Note: For full teardown, we should ideally check `table_empty`
        // and free upper levels. To keep it simple in this iteration,
        // we omit immediate pruning here or rely on space destroy.
        x86_tlb_flush_page_local(aligned_vaddr);
    }

    return 0;
}

static int x86_pt_protect_4k(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    page_table_walk_result_t res;
    int rc = x86_pt_walk(root_pt, vaddr, false, 0, &res);
    if (rc != 0) return rc;

    if (!res.present) return -2;

    if (res.entry_va) {
        pte_raw_t* pte = (pte_raw_t*)res.entry_va;
        uint64_t pte_flags = flags_to_x86(new_flags);
        if (res.is_large && res.level == 2) {
            pte_flags |= X86_PT_HUGE;
        }
        *pte = res.mapped_pa | pte_flags;
        x86_tlb_flush_page_local(aligned_vaddr);
        return 0;
    }

    return -1;
}

static int x86_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    page_table_walk_result_t res;
    int rc = x86_pt_walk(root_pt, vaddr, false, 0, &res);
    if (rc != 0) return rc;

    if (!res.present) return -2;

    if (paddr) {
        if (res.is_large && res.level == 2) {
            uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;
            *paddr = res.mapped_pa + (pt_idx * PAGE_SIZE);
        } else {
            *paddr = res.mapped_pa;
        }
    }

    if (flags) {
        *flags = res.flags;
    }

    return 0;
}

static int x86_pt_map_large_2m(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U) return -1;

    // We pass HAL_PT_FLAG_LARGE_2M to x86_pt_walk so it knows to stop at level 2
    uint32_t alloc_flags = flags | HAL_PT_FLAG_LARGE_2M;

    page_table_walk_result_t res;
    int rc = x86_pt_walk(root_pt, vaddr, true, alloc_flags, &res);
    if (rc != 0) return rc;

    // res.level should be 2 for a 2M page
    if (res.level == 2 && res.entry_va) {
        pte_raw_t* pte = (pte_raw_t*)res.entry_va;

        // If there was a previous 4K page table there, it was NOT freed by the walker.
        // We handle the cleanup here manually if necessary.
        if (res.present && !res.is_large) {
             pt_cache_free((*pte) & X86_PAGE_MASK);
        }

        *pte = (paddr & ~((1ULL << 21) - 1ULL)) | flags_to_x86(flags) | X86_PT_HUGE;
        return 0;
    }
    return -1;
}

static int x86_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    size_t done = 0;
    while (done < size) {
        size_t remaining = size - done;
        bool use_2m = (flags & HAL_PT_FLAG_LARGE_2M) &&
                      remaining >= X86_LARGE_2M_SIZE &&
                      huge_2m_aligned(vaddr + done, paddr + done);
        int rc = use_2m
            ? x86_pt_map_large_2m(root_pt, vaddr + done, paddr + done, flags)
            : x86_pt_map_4k(root_pt, vaddr + done, paddr + done, flags & ~HAL_PT_FLAG_LARGE_2M);
        if (rc != 0) {
            return rc;
        }
        done += use_2m ? X86_LARGE_2M_SIZE : PAGE_SIZE;
    }
    return 0;
}

static int x86_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    size_t done = 0;
    while (done < size) {
        int rc = x86_pt_unmap_4k(root_pt, vaddr + done, NULL);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int x86_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    size_t done = 0;
    while (done < size) {
        int rc = x86_pt_protect_4k(root_pt, vaddr + done, new_flags);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int x86_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return x86_pt_map_range(root_pt, vaddr, paddr, PAGE_SIZE, flags & ~HAL_PT_FLAG_LARGE_2M);
}

static int x86_pt_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (unmapped_paddr) {
        (void)x86_pt_query_page(root_pt, vaddr, unmapped_paddr, NULL);
    }
    int rc = x86_pt_unmap_range(root_pt, vaddr, PAGE_SIZE);
    return rc;
}

static int x86_pt_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    return x86_pt_protect_range(root_pt, vaddr, PAGE_SIZE, new_flags);
}

static int x86_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
    int rc = x86_pt_query_page(root_pt, vaddr, paddr, flags);
    if (rc != 0) return rc;
    if (mapped_size) {
        *mapped_size = (flags && (*flags & HAL_PT_FLAG_LARGE_2M)) ? X86_LARGE_2M_SIZE : PAGE_SIZE;
    }
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

    // Enable NX (No-Execute) in EFER (Extended Feature Enable Register)
    // MSR 0xC0000080, Bit 11
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xC0000080));
    low |= (1 << 11);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(0xC0000080));
}

static translate_backend_kind_t x86_backend_type(void) { return TRANSLATE_BACKEND_MMU; }
static translate_exec_class_t x86_exec_class(void) { return TRANSLATE_EXEC_MMU_FULL; }

void x86_pt_set_mmu_finalized(bool finalized) {
    g_x86_mmu_finalized = finalized;
}

static phys_addr_t x86_virt_to_phys(const void* virt) { 
    if (!g_x86_mmu_finalized) {
        return (phys_addr_t)(uintptr_t)virt;
    }
    return (phys_addr_t)((uintptr_t)virt - g_kernel_virt_offset); 
}
static bool x86_has_linear_physmap(void) { return g_x86_mmu_finalized; }
static phys_addr_t x86_linear_physmap_base(void) { 
    return g_x86_mmu_finalized ? g_kernel_virt_offset : 0; 
}
static phys_addr_t x86_linear_physmap_limit(void) { 
    return g_x86_mmu_finalized ? (g_kernel_virt_offset + g_kernel_physmap_size) : 0; 
}

static const hal_translate_ops_t x86_translate_ops = {
    .backend_type = x86_backend_type,
    .exec_class = x86_exec_class,
    .phys_to_virt = x86_phys_to_virt,
    .virt_to_phys = x86_virt_to_phys,
    .has_linear_physmap = x86_has_linear_physmap,
    .linear_physmap_base = x86_linear_physmap_base,
    .linear_physmap_limit = x86_linear_physmap_limit,
};

const hal_translate_ops_t* hal_translate_ops(void) {
    return &x86_translate_ops;
}

static hal_pt_caps_t x86_pt_caps = {
    .backend_kind = TRANSLATE_BACKEND_MMU,
    .exec_class = TRANSLATE_EXEC_MMU_FULL,
    .supports_sparse_vm = true,
    .supports_demand_fault = true,
    .supports_protect = true,
    .supports_query = true,
    .supports_range_map = true,
    .supports_range_unmap = true,
    .supports_range_protect = true,
    .supports_asid = false,
    .supports_pcid = false,
    .supports_global = true,
    .supports_nx_or_xn = true,
    .supports_ad_bits = true,
    .supports_large_2m = true,
    .supports_large_1g = false,
    .supports_device_memtype = true,
    .supports_writecombine = false,
    .requires_bbm = false,
    .supports_cow_softbit = true,
    .supports_linear_physmap = true,
};

hal_pt_ops_t x86_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MMU,
    .caps                  = &x86_pt_caps,
    .create_address_space  = x86_pt_create_address_space,
    .destroy_address_space = x86_pt_destroy_address_space,
    .map_page              = x86_pt_map_page,
    .unmap_page            = x86_pt_unmap_page,
    .protect_page          = x86_pt_protect_page,
    .query_page            = x86_pt_query_page,
    .map_range             = x86_pt_map_range,
    .unmap_range           = x86_pt_unmap_range,
    .protect_range         = x86_pt_protect_range,
    .query_mapping         = x86_pt_query_mapping,
};

// --- x86_64 TLB Operations ---

static void x86_tlb_flush_page_local(virt_addr_t vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

static void x86_tlb_flush_all_local(void) {
    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    arch_tlb_prepare_full_flush_cr3(&cr3);
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

static void x86_tlb_flush_asid_local(uint16_t asid) {
    if (!arch_tlb_flush_asid(asid)) {
        x86_tlb_flush_all_local();
    }
}

static void x86_tlb_flush_range_local(virt_addr_t start, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        x86_tlb_flush_page_local(start + offset);
        offset += PAGE_SIZE; // assuming PAGE_SIZE is available or define it
    }
}

static void x86_tlb_flush_page_remote(uint16_t target_core, uint16_t asid, virt_addr_t vaddr) {
    // Legacy support via coordinator or direct if implemented
    (void)target_core; (void)asid; (void)vaddr;
}

static void x86_tlb_flush_range_remote(uint16_t target_core, uint16_t asid, virt_addr_t start, size_t len) {
    (void)target_core; (void)asid; (void)start; (void)len;
}

static void x86_tlb_flush_all_remote(uint16_t target_core, uint16_t asid) {
    (void)target_core; (void)asid;
}

static void x86_tlb_flush_page_broadcast(uint16_t asid, virt_addr_t vaddr) {
    (void)asid; (void)vaddr;
}

static void x86_tlb_flush_range_broadcast(uint16_t asid, virt_addr_t start, size_t len) {
    (void)asid; (void)start; (void)len;
}

static void x86_tlb_flush_all_broadcast(uint16_t asid) {
    (void)asid;
}

static hal_tlb_caps_t x86_tlb_caps = {
    .supports_page_flush = true,
    .supports_range_flush = true,
    .supports_aspace_flush = true,
    .supports_all_flush = true,
    .supports_remote_targeted_flush = false,
    .supports_broadcast_flush = false,
    .supports_asid_selective_flush = false,
    .supports_lazy_generation_model = false,
};

hal_tlb_ops_t x86_hal_tlb_ops = {
    .caps                 = &x86_tlb_caps,
    .flush_page_local      = x86_tlb_flush_page_local,
    .flush_range_local     = x86_tlb_flush_range_local,
    .flush_all_local       = x86_tlb_flush_all_local,
    .flush_asid_local      = x86_tlb_flush_asid_local,
    .flush_page_remote     = x86_tlb_flush_page_remote,
    .flush_range_remote    = x86_tlb_flush_range_remote,
    .flush_all_remote      = x86_tlb_flush_all_remote,
    .flush_page_broadcast  = x86_tlb_flush_page_broadcast,
    .flush_range_broadcast = x86_tlb_flush_range_broadcast,
    .flush_all_broadcast   = x86_tlb_flush_all_broadcast,
};

void x86_pt_caps_init(void) {
    g_x86_pcid_supported = arch_cpu_has_system_all(ARCH_CPU_FEAT_X86_PCID);
    if (g_x86_pcid_supported) {
        x86_pt_caps.supports_pcid = true;
        x86_tlb_caps.supports_asid_selective_flush = true;
    }
}

// Implement the specific functions from mem_protect_cpu_ops

static phys_addr_t x86_mpa_make_table(uint32_t level) {
    (void)level;
    phys_addr_t pt = pt_cache_alloc();
    if (pt) {
        pt_t* ptr = (pt_t*)(uintptr_t)pt;
        for(int i = 0; i < 512; i++) ptr->entries[i] = 0;
    }
    return pt;
}

static int x86_mpa_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    uint32_t hal_flags = HAL_PT_FLAG_READ;
    if (flags & MPA_CAP_EXEC_PERM) hal_flags |= HAL_PT_FLAG_EXEC;
    if (flags & MPA_CAP_USER) hal_flags |= HAL_PT_FLAG_USER;
    if (flags & MPA_CAP_GLOBAL) hal_flags |= HAL_PT_FLAG_GLOBAL;
    if (flags & MPA_CAP_WRITE) hal_flags |= HAL_PT_FLAG_WRITE;

    return x86_pt_map_4k(root_pt, vaddr, paddr, hal_flags);
}

static int x86_mpa_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    return x86_pt_unmap_4k(root_pt, vaddr, unmapped_paddr);
}

static void x86_mpa_set_root(phys_addr_t root) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(root) : "memory");
}

static void x86_mpa_flush_tlb_local(virt_addr_t vaddr, uint16_t asid) {
    if (arch_tlb_flush_addr_asid((uintptr_t)vaddr, asid)) {
        return;
    }

    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

static phys_addr_t x86_mpa_get_root(void) {
    phys_addr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & ~(0xFFFULL);
}

mem_protect_ops_t x86_mem_protect_ops = {
    .supported_caps = MPA_CAP_VIRT | MPA_CAP_GLOBAL | MPA_CAP_HUGEPAGE | MPA_CAP_EXEC_PERM | MPA_CAP_USER,
    .cpu_ops = {
        .make_table = x86_mpa_make_table,
        .map_page = x86_mpa_map_page,
        .unmap_page = x86_mpa_unmap_page,
        .set_root = x86_mpa_set_root,
        .flush_tlb_local = x86_mpa_flush_tlb_local,
        .get_root = x86_mpa_get_root,
    },
    .iommu_ops = {
        .probe = NULL, // Could be linked to VTD probe later
    }
};

mem_protect_ops_t *active_mem_protect = &x86_mem_protect_ops;
