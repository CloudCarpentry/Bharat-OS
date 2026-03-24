#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_pt_walk.h"
#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm.h"
#include "../../kernel/include/numa.h"
#include "../../kernel/include/mm/physmap.h"
#include <stdbool.h>

// ARMv7 Short-Descriptor Page Table Entry bits
#define ARM32_PT_L1_TYPE_FAULT 0x0
#define ARM32_PT_L1_TYPE_PAGE  0x1
#define ARM32_PT_L1_TYPE_SECT  0x2

#define ARM32_PT_L2_TYPE_FAULT 0x0
#define ARM32_PT_L2_TYPE_LARGE 0x1
#define ARM32_PT_L2_TYPE_SMALL 0x2

#define ARM32_PT_L2_XN (1U << 0) // Execute-Never
#define ARM32_PT_L2_B  (1U << 2) // Bufferable
#define ARM32_PT_L2_C  (1U << 3) // Cacheable
#define ARM32_PT_L2_AP0 (1U << 4) // Access Permission bit 0
#define ARM32_PT_L2_AP1 (1U << 5) // Access Permission bit 1
#define ARM32_PT_L2_AP2 (1U << 9) // Access Permission bit 2
#define ARM32_PT_L2_S   (1U << 10) // Shared
#define ARM32_PT_L2_nG  (1U << 11) // Not Global

#define ARM32_PAGE_MASK (~0xFFFU)

typedef uint32_t pte_raw_t;

typedef struct {
    pte_raw_t entries[4096]; // 4096 entries * 4 bytes = 16KB L1 table
} pt_l1_t;

typedef struct {
    pte_raw_t entries[256]; // 256 entries * 4 bytes = 1KB L2 table
} pt_l2_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & ARM32_PAGE_MASK;
}

static bool table_empty(pt_l2_t* table) {
    for (size_t i = 0; i < 256; i++) {
        if ((table->entries[i] & 0x3) != ARM32_PT_L2_TYPE_FAULT) {
            return false;
        }
    }
    return true;
}

static uint32_t flags_to_arm32(uint32_t flags) {
    // Basic AP mapping (assuming AP[2:0] = 0b011 for full access in modern AP setup)
    // AP[2] (bit 9) = 0 for R/W, 1 for RO
    // AP[1:0] (bits 5:4) = 01 (Privileged access only), 11 (User & Privileged)
    uint32_t pte_flags = ARM32_PT_L2_TYPE_SMALL;

    // Default Cacheable/Bufferable (Write-Back)
    pte_flags |= ARM32_PT_L2_C | ARM32_PT_L2_B;

    if (flags & HAL_PT_FLAG_DEVICE) {
        pte_flags &= ~(ARM32_PT_L2_C | ARM32_PT_L2_B); // Strictly ordered
    }

    if (flags & HAL_PT_FLAG_USER) {
        if (flags & HAL_PT_FLAG_WRITE) {
            pte_flags |= ARM32_PT_L2_AP1 | ARM32_PT_L2_AP0; // 0b11 (User R/W, Priv R/W)
        } else {
            pte_flags |= ARM32_PT_L2_AP2 | ARM32_PT_L2_AP1 | ARM32_PT_L2_AP0; // 0b11 + AP[2]=1 (User RO, Priv RO)
        }
    } else {
        if (flags & HAL_PT_FLAG_WRITE) {
            pte_flags |= ARM32_PT_L2_AP0; // 0b01 (User None, Priv R/W)
        } else {
            pte_flags |= ARM32_PT_L2_AP2 | ARM32_PT_L2_AP0; // 0b01 + AP[2]=1 (User None, Priv RO)
        }
    }

    if (!(flags & HAL_PT_FLAG_EXEC)) {
        pte_flags |= ARM32_PT_L2_XN;
    }

    if (!(flags & HAL_PT_FLAG_GLOBAL)) {
        pte_flags |= ARM32_PT_L2_nG;
    }

    return pte_flags;
}

static uint32_t arm32_to_flags(uint32_t pte_flags) {
    uint32_t flags = HAL_PT_FLAG_READ;

    // Check AP bits
    uint8_t ap = ((pte_flags & ARM32_PT_L2_AP2) ? 4 : 0) |
                 ((pte_flags & ARM32_PT_L2_AP1) ? 2 : 0) |
                 ((pte_flags & ARM32_PT_L2_AP0) ? 1 : 0);

    switch(ap) {
        case 1: // 0b001 Priv R/W
            flags |= HAL_PT_FLAG_WRITE;
            break;
        case 3: // 0b011 Priv R/W, User R/W
            flags |= HAL_PT_FLAG_WRITE | HAL_PT_FLAG_USER;
            break;
        case 5: // 0b101 Priv RO
            break;
        case 7: // 0b111 Priv RO, User RO
            flags |= HAL_PT_FLAG_USER;
            break;
    }

    if (!(pte_flags & ARM32_PT_L2_XN)) flags |= HAL_PT_FLAG_EXEC;
    if (!(pte_flags & ARM32_PT_L2_nG)) flags |= HAL_PT_FLAG_GLOBAL;
    if (!(pte_flags & (ARM32_PT_L2_C | ARM32_PT_L2_B))) flags |= HAL_PT_FLAG_DEVICE;

    return flags;
}

// Scaffold backend for ARM32 (MMU-Lite)
// This implements TRANSLATE_EXEC_MMU_LITE where the kernel has a small permanent
// window and full identity map is not possible.

static translate_backend_kind_t arm32_backend_type(void) { return TRANSLATE_BACKEND_MMU; }
static translate_exec_class_t arm32_exec_class(void) { return TRANSLATE_EXEC_MMU_LITE; }

static void* arm32_phys_to_virt(phys_addr_t phys) { (void)phys; return NULL; /* Needs dynamic kmap for non-linear */ }
static phys_addr_t arm32_virt_to_phys(const void* virt) { (void)virt; return 0; }
static bool arm32_has_linear_physmap(void) { return false; }
static phys_addr_t arm32_linear_physmap_base(void) { return 0; }
static phys_addr_t arm32_linear_physmap_limit(void) { return 0; }

static const hal_translate_ops_t arm32_translate_ops = {
    .backend_type = arm32_backend_type,
    .exec_class = arm32_exec_class,
    .phys_to_virt = arm32_phys_to_virt,
    .virt_to_phys = arm32_virt_to_phys,
    .has_linear_physmap = arm32_has_linear_physmap,
    .linear_physmap_base = arm32_linear_physmap_base,
    .linear_physmap_limit = arm32_linear_physmap_limit,
};

static phys_addr_t arm32_create_address_space(phys_addr_t kernel_root_table) {
    // ARM32 L1 is 16KB, requires 4 contiguous 4K pages
    pmm_block_t block;
    if (pmm_alloc_contiguous(4, PMM_ZONE_ANY, PMM_ALLOC_ZERO, &block) != 0) {
        return 0;
    }

    phys_addr_t root = block.phys_addr;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root; // Assume identity map for early tests

    if (kernel_root_table != 0U) {
        pt_l1_t* kernel_l1 = (pt_l1_t*)(uintptr_t)kernel_root_table;
        // Typically higher half is kernel, copy 0x80000000 -> 0xFFFFFFFF (entries 2048->4095)
        for(int i = 2048; i < 4096; i++) {
            l1_table->entries[i] = kernel_l1->entries[i];
        }
    }

    return root;
}

static void arm32_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        pt_l1_t* l1 = (pt_l1_t*)(uintptr_t)root_pt;
        for (int i = 0; i < 2048; i++) { // Free only User half L2 tables
            if ((l1->entries[i] & 0x3) == ARM32_PT_L1_TYPE_PAGE) {
                mm_free_page(l1->entries[i] & 0xFFFFFC00);
            }
        }
        pmm_block_t block;
        block.phys_addr = root_pt;
        block.page_count = 4;
        block.order = 2;
        pmm_free_pages(&block);
    }
}

static int arm32_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;

    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) {
        phys_addr_t new_l2 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l2) return -2;
        pt_l2_t* l2_ptr = (pt_l2_t*)(uintptr_t)new_l2;
        for(int i=0; i<256; i++) l2_ptr->entries[i] = 0;
        l1_table->entries[l1_idx] = (new_l2 & 0xFFFFFC00) | ARM32_PT_L1_TYPE_PAGE;
    }

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    uint32_t pte_flags = flags_to_arm32(flags);
    l2_table->entries[l2_idx] = (aligned_paddr & 0xFFFFF000) | pte_flags;

    return 0;
}

static int arm32_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if ((l2_table->entries[l2_idx] & 0x3) != ARM32_PT_L2_TYPE_SMALL) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    }

    l2_table->entries[l2_idx] = 0;

    if (table_empty(l2_table)) {
        mm_free_page(l1_table->entries[l1_idx] & 0xFFFFFC00);
        l1_table->entries[l1_idx] = 0;
    }

    return 0;
}

static int arm32_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if ((l2_table->entries[l2_idx] & 0x3) != ARM32_PT_L2_TYPE_SMALL) return -2;

    uint32_t paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    uint32_t pte_flags = flags_to_arm32(new_flags);

    l2_table->entries[l2_idx] = paddr | pte_flags;

    return 0;
}

static int arm32_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if ((l2_table->entries[l2_idx] & 0x3) != ARM32_PT_L2_TYPE_SMALL) return -2;

    if (paddr) *paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    if (flags) *flags = arm32_to_flags(l2_table->entries[l2_idx] & ~0xFFFFF000);

    return 0;
}

static const hal_pt_caps_t arm32_pt_caps = {
    .backend_kind = TRANSLATE_BACKEND_MMU,
    .exec_class = TRANSLATE_EXEC_MMU_LITE,
    .supports_sparse_vm = true,
    .supports_demand_fault = true,
    .supports_protect = true,
    .supports_query = true,
    .supports_range_map = false,
    .supports_range_unmap = false,
    .supports_range_protect = false,
    .supports_asid = true,
    .supports_pcid = false,
    .supports_global = true,
    .supports_nx_or_xn = true,
    .supports_ad_bits = false,
    .supports_large_2m = false,
    .supports_large_1g = false,
    .supports_device_memtype = true,
    .supports_writecombine = false,
    .requires_bbm = false,
    .supports_cow_softbit = false,
    .supports_linear_physmap = false,
};

hal_pt_ops_t arm32_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MMU,
    .caps                  = &arm32_pt_caps,
    .create_address_space  = arm32_create_address_space,
    .destroy_address_space = arm32_destroy_address_space,
    .map_page              = arm32_map_page,
    .unmap_page            = arm32_unmap_page,
    .protect_page          = arm32_protect_page,
    .query_page            = arm32_query_page,
    .map_range             = NULL,
    .unmap_range           = NULL,
    .protect_range         = NULL,
    .query_mapping         = NULL,
};

static void arm32_tlb_flush_page_local(virt_addr_t vaddr) {
    asm volatile("mcr p15, 0, %0, c8, c7, 1" : : "r" (vaddr));
    asm volatile("dsb");
    asm volatile("isb");
}

static void arm32_tlb_flush_all_local(void) {
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));
    asm volatile("dsb");
    asm volatile("isb");
}

static void arm32_tlb_flush_asid_local(uint16_t asid) {
    asm volatile("mcr p15, 0, %0, c8, c7, 2" : : "r" (asid));
    asm volatile("dsb");
    asm volatile("isb");
}

static const hal_tlb_caps_t arm32_tlb_caps = {
    .supports_page_flush = true,
    .supports_range_flush = false,
    .supports_aspace_flush = true,
    .supports_all_flush = true,
    .supports_remote_targeted_flush = false,
    .supports_broadcast_flush = false,
    .supports_asid_selective_flush = true,
    .supports_lazy_generation_model = false,
};

hal_tlb_ops_t arm32_hal_tlb_ops = {
    .caps                 = &arm32_tlb_caps,
    .flush_page_local      = arm32_tlb_flush_page_local,
    .flush_range_local     = NULL,
    .flush_all_local       = arm32_tlb_flush_all_local,
    .flush_asid_local      = arm32_tlb_flush_asid_local,
    .flush_page_remote     = NULL,
    .flush_range_remote    = NULL,
    .flush_all_remote      = NULL,
    .flush_page_broadcast  = NULL,
    .flush_range_broadcast = NULL,
    .flush_all_broadcast   = NULL,
};

// Override symbol to avoid redefinition if built alongside others
#if defined(__arm__) && !defined(__aarch64__)
const hal_translate_ops_t* hal_translate_ops(void) {
    return &arm32_translate_ops;
}
#endif