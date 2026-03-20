#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm.h"
#include "../../../include/numa.h"
#include "../../../include/mm/physmap.h"
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

#define X86_PAGE_MASK   (~0xFFFULL)
#define X86_LARGE_2M_SIZE (1ULL << 21)

// Arch-private raw descriptor
typedef uint64_t pte_raw_t;

typedef struct {
    pte_raw_t entries[512];
} pt_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & X86_PAGE_MASK;
}

static bool table_empty(pt_t* table) {
    for (size_t i = 0; i < 512; i++) {
        if (table->entries[i] & X86_PT_PRESENT) {
            return false;
        }
    }
    return true;
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
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pt_t* pml4 = (pt_t*)phys_to_virt_linear(root);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    phys_addr_t kernel_root = kernel_root_table;
    if (kernel_root != 0U) {
        pt_t* kernel_pml4 = (pt_t*)phys_to_virt_linear(kernel_root);
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
        pt_t* pt = (pt_t*)phys_to_virt_linear(table);
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

static void x86_pt_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        x86_pt_destroy_recursive(root_pt, 4);
    }
}

static int x86_pt_map_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)phys_to_virt_linear(root_pt);

    // Provide generic permissive access (RW | User) to intermediate tables
    // The leaf PTE restricts the final access permissions.
    uint64_t dir_flags = X86_PT_PRESENT | X86_PT_RW | X86_PT_USER;

    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pdp = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pdp) return -2;
        pt_t* pdp_ptr = (pt_t*)phys_to_virt_linear(new_pdp);
        for(int i=0; i<512; i++) pdp_ptr->entries[i] = 0;
        pml4->entries[pml4_idx] = new_pdp | dir_flags;
    }

    pt_t* pdp = (pt_t*)phys_to_virt_linear(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pd) return -2;
        pt_t* pd_ptr = (pt_t*)phys_to_virt_linear(new_pd);
        for(int i=0; i<512; i++) pd_ptr->entries[i] = 0;
        pdp->entries[pdp_idx] = new_pd | dir_flags;
    }

    pt_t* pd = (pt_t*)phys_to_virt_linear(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pt = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pt) return -2;
        pt_t* pt_ptr = (pt_t*)phys_to_virt_linear(new_pt);
        for(int i=0; i<512; i++) pt_ptr->entries[i] = 0;
        pd->entries[pd_idx] = new_pt | dir_flags;
    } else if (pd->entries[pd_idx] & X86_PT_HUGE) {
        phys_addr_t huge_base = pd->entries[pd_idx] & ~((1ULL << 21) - 1ULL);
        phys_addr_t new_pt = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pt) return -2;
        pt_t* split_pt = (pt_t*)phys_to_virt_linear(new_pt);
        uint64_t split_flags = (pd->entries[pd_idx] & ~X86_PAGE_MASK) & ~X86_PT_HUGE;
        for (size_t i = 0; i < 512; i++) {
            split_pt->entries[i] = (huge_base + (i * PAGE_SIZE)) | split_flags;
        }
        pd->entries[pd_idx] = new_pt | dir_flags;
    }

    pt_t* pt = (pt_t*)phys_to_virt_linear(pd->entries[pd_idx] & X86_PAGE_MASK);

    uint64_t pte_flags = flags_to_x86(flags);
    pt->entries[pt_idx] = aligned_paddr | pte_flags;

    return 0;
}

static int x86_pt_unmap_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)phys_to_virt_linear(root_pt);
    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pdp = (pt_t*)phys_to_virt_linear(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pd = (pt_t*)phys_to_virt_linear(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) return -2;
    if (pd->entries[pd_idx] & X86_PT_HUGE) {
        if (unmapped_paddr) {
            *unmapped_paddr = (pd->entries[pd_idx] & ~((1ULL << 21) - 1ULL)) + (pt_idx * PAGE_SIZE);
        }
        pd->entries[pd_idx] = 0;
        if (table_empty(pd)) {
            mm_free_page(pdp->entries[pdp_idx] & X86_PAGE_MASK);
            pdp->entries[pdp_idx] = 0;
            if (table_empty(pdp)) {
                mm_free_page(pml4->entries[pml4_idx] & X86_PAGE_MASK);
                pml4->entries[pml4_idx] = 0;
            }
        }
        return 0;
    }
    pt_t* pt = (pt_t*)phys_to_virt_linear(pd->entries[pd_idx] & X86_PAGE_MASK);

    if ((pt->entries[pt_idx] & X86_PT_PRESENT) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = pt->entries[pt_idx] & X86_PAGE_MASK;
    }

    pt->entries[pt_idx] = 0;

    if (table_empty(pt)) {
        mm_free_page(pd->entries[pd_idx] & X86_PAGE_MASK);
        pd->entries[pd_idx] = 0;
        if (table_empty(pd)) {
            mm_free_page(pdp->entries[pdp_idx] & X86_PAGE_MASK);
            pdp->entries[pdp_idx] = 0;
            if (table_empty(pdp)) {
                mm_free_page(pml4->entries[pml4_idx] & X86_PAGE_MASK);
                pml4->entries[pml4_idx] = 0;
            }
        }
    }

    return 0;
}

static int x86_pt_protect_4k(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)phys_to_virt_linear(root_pt);
    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pdp = (pt_t*)phys_to_virt_linear(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pd = (pt_t*)phys_to_virt_linear(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) return -2;
    if (pd->entries[pd_idx] & X86_PT_HUGE) {
        uint64_t paddr_2m = pd->entries[pd_idx] & ~((1ULL << 21) - 1ULL);
        uint64_t pte_flags = flags_to_x86(new_flags) | X86_PT_HUGE;
        pd->entries[pd_idx] = paddr_2m | pte_flags;
        return 0;
    }
    pt_t* pt = (pt_t*)phys_to_virt_linear(pd->entries[pd_idx] & X86_PAGE_MASK);

    if ((pt->entries[pt_idx] & X86_PT_PRESENT) == 0) return -2;

    uint64_t paddr = pt->entries[pt_idx] & X86_PAGE_MASK;
    uint64_t pte_flags = flags_to_x86(new_flags);

    pt->entries[pt_idx] = paddr | pte_flags;

    return 0;
}

static int x86_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pml4 = (pt_t*)phys_to_virt_linear(root_pt);
    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pdp = (pt_t*)phys_to_virt_linear(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) return -2;
    pt_t* pd = (pt_t*)phys_to_virt_linear(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) == 0) return -2;
    if (pd->entries[pd_idx] & X86_PT_HUGE) {
        if (paddr) *paddr = (pd->entries[pd_idx] & ~((1ULL << 21) - 1ULL)) + (pt_idx * PAGE_SIZE);
        if (flags) {
            *flags = x86_to_flags(pd->entries[pd_idx] & ~X86_PAGE_MASK);
            *flags |= HAL_PT_FLAG_LARGE_2M;
        }
        return 0;
    }
    pt_t* pt = (pt_t*)phys_to_virt_linear(pd->entries[pd_idx] & X86_PAGE_MASK);

    if ((pt->entries[pt_idx] & X86_PT_PRESENT) == 0) return -2;

    if (paddr) *paddr = pt->entries[pt_idx] & X86_PAGE_MASK;
    if (flags) *flags = x86_to_flags(pt->entries[pt_idx] & ~X86_PAGE_MASK);

    return 0;
}

static int x86_pt_map_large_2m(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
    pt_t* pml4 = (pt_t*)phys_to_virt_linear(root_pt);
    uint64_t dir_flags = X86_PT_PRESENT | X86_PT_RW | X86_PT_USER;

    if ((pml4->entries[pml4_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pdp = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pdp) return -2;
        pt_t* pdp_ptr = (pt_t*)phys_to_virt_linear(new_pdp);
        for (int i = 0; i < 512; i++) pdp_ptr->entries[i] = 0;
        pml4->entries[pml4_idx] = new_pdp | dir_flags;
    }
    pt_t* pdp = (pt_t*)phys_to_virt_linear(pml4->entries[pml4_idx] & X86_PAGE_MASK);
    if ((pdp->entries[pdp_idx] & X86_PT_PRESENT) == 0) {
        phys_addr_t new_pd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pd) return -2;
        pt_t* pd_ptr = (pt_t*)phys_to_virt_linear(new_pd);
        for (int i = 0; i < 512; i++) pd_ptr->entries[i] = 0;
        pdp->entries[pdp_idx] = new_pd | dir_flags;
    }
    pt_t* pd = (pt_t*)phys_to_virt_linear(pdp->entries[pdp_idx] & X86_PAGE_MASK);
    if ((pd->entries[pd_idx] & X86_PT_PRESENT) && !(pd->entries[pd_idx] & X86_PT_HUGE)) {
        mm_free_page(pd->entries[pd_idx] & X86_PAGE_MASK);
    }
    pd->entries[pd_idx] = (paddr & ~((1ULL << 21) - 1ULL)) | flags_to_x86(flags) | X86_PT_HUGE;
    return 0;
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
}

hal_pt_ops_t x86_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MMU,
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

hal_tlb_ops_t x86_hal_tlb_ops = {
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
