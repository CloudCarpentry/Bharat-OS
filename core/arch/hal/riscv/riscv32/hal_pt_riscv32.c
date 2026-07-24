#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_pt_walk.h"
#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/hal/hal_mpa.h"
#include "../../kernel/include/mm.h"
#include "../../kernel/include/numa.h"
#include "../../kernel/include/mm/physmap.h"
#include <stdbool.h>

// RV32 Sv32 Page Table Entry bits
#define RISCV32_PT_V (1UL << 0) // Valid
#define RISCV32_PT_R (1UL << 1) // Read
#define RISCV32_PT_W (1UL << 2) // Write
#define RISCV32_PT_X (1UL << 3) // Execute
#define RISCV32_PT_U (1UL << 4) // User
#define RISCV32_PT_G (1UL << 5) // Global
#define RISCV32_PT_A (1UL << 6) // Accessed
#define RISCV32_PT_D (1UL << 7) // Dirty

#define RISCV32_PAGE_MASK (~0xFFFUL)

typedef uint32_t pte_raw_t;

typedef struct {
    pte_raw_t entries[1024];
} pt_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & RISCV32_PAGE_MASK;
}

static bool table_empty(pt_t* table) {
    for (size_t i = 0; i < 1024; i++) {
        if (table->entries[i] & RISCV32_PT_V) {
            return false;
        }
    }
    return true;
}

static uint32_t flags_to_riscv(uint32_t flags) {
    uint32_t pte_flags = RISCV32_PT_V | RISCV32_PT_A | RISCV32_PT_D;

    if (flags & HAL_PT_FLAG_READ)  pte_flags |= RISCV32_PT_R;
    if (flags & HAL_PT_FLAG_WRITE) pte_flags |= RISCV32_PT_W;
    if (flags & HAL_PT_FLAG_EXEC)  pte_flags |= RISCV32_PT_X;
    if (flags & HAL_PT_FLAG_USER)  pte_flags |= RISCV32_PT_U;
    if (flags & HAL_PT_FLAG_GLOBAL) pte_flags |= RISCV32_PT_G;

    return pte_flags;
}

static uint32_t riscv_to_flags(uint32_t pte_flags) {
    uint32_t flags = 0;

    if (pte_flags & RISCV32_PT_R) flags |= HAL_PT_FLAG_READ;
    if (pte_flags & RISCV32_PT_W) flags |= HAL_PT_FLAG_WRITE;
    if (pte_flags & RISCV32_PT_X) flags |= HAL_PT_FLAG_EXEC;
    if (pte_flags & RISCV32_PT_U) flags |= HAL_PT_FLAG_USER;
    if (pte_flags & RISCV32_PT_G) flags |= HAL_PT_FLAG_GLOBAL;

    return flags;
}

// Scaffold backend for RISCV32 (MMU-Lite)
// This implements TRANSLATE_EXEC_MMU_LITE where the kernel has a small permanent
// window and full identity map is not possible.

static translate_backend_kind_t riscv32_backend_type(void) { return TRANSLATE_BACKEND_MMU; }
static translate_exec_class_t riscv32_exec_class(void) { return TRANSLATE_EXEC_MMU_LITE; }

// We don't have a direct map on RV32 typically, assuming a specific kernel layout or no linear map
static void* riscv32_phys_to_virt(phys_addr_t phys) { (void)phys; return NULL; /* Needs dynamic kmap for non-linear */ }
static phys_addr_t riscv32_virt_to_phys(const void* virt) { (void)virt; return 0; }
static bool riscv32_has_linear_physmap(void) { return false; }
static phys_addr_t riscv32_linear_physmap_base(void) { return 0; }
static phys_addr_t riscv32_linear_physmap_limit(void) { return 0; }

static const hal_translate_ops_t riscv32_translate_ops = {
    .backend_type = riscv32_backend_type,
    .exec_class = riscv32_exec_class,
    .phys_to_virt = riscv32_phys_to_virt,
    .virt_to_phys = riscv32_virt_to_phys,
    .has_linear_physmap = riscv32_has_linear_physmap,
    .linear_physmap_base = riscv32_linear_physmap_base,
    .linear_physmap_limit = riscv32_linear_physmap_limit,
};

static phys_addr_t riscv32_create_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pt_t* l1_table = (pt_t*)(uintptr_t)root; // Note: In absence of phys_to_virt, we assume direct identity mapped memory for root PT in tests/early boot
    for (int i = 0; i < 1024; i++) {
        l1_table->entries[i] = 0;
    }

    if (kernel_root_table != 0U) {
        pt_t* kernel_l1 = (pt_t*)(uintptr_t)kernel_root_table;
        for(int i = 512; i < 1024; i++) {
            l1_table->entries[i] = kernel_l1->entries[i];
        }
    }

    return root;
}

static void riscv32_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pt_t* pt = (pt_t*)(uintptr_t)table;
        int max_idx = (level == 1) ? 512 : 1024; // Free only bottom half of L1
        for (int i = 0; i < max_idx; i++) {
            if (pt->entries[i] & RISCV32_PT_V) {
                if ((pt->entries[i] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) == 0) {
                    riscv32_pt_destroy_recursive((pt->entries[i] >> 10) << 12, level - 1);
                }
            }
        }
    }
    mm_free_page(table);
}

static void riscv32_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        riscv32_pt_destroy_recursive(root_pt, 1);
    }
}

static int riscv32_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;

    uint32_t table_flags = RISCV32_PT_V;

    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) {
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pt_t* l0_ptr = (pt_t*)(uintptr_t)new_l0;
        for(int i=0; i<1024; i++) l0_ptr->entries[i] = 0;
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | table_flags;
    }

    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    uint32_t pte_flags = flags_to_riscv(flags);
    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

    return 0;
}

static int riscv32_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = (l0_table->entries[vpn0] >> 10) << 12;
    }

    l0_table->entries[vpn0] = 0;

    if (table_empty(l0_table)) {
        mm_free_page((l1_table->entries[vpn1] >> 10) << 12);
        l1_table->entries[vpn1] = 0;
    }

    return 0;
}

static int riscv32_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    uint32_t paddr = (l0_table->entries[vpn0] >> 10) << 12;
    uint32_t pte_flags = flags_to_riscv(new_flags);

    l0_table->entries[vpn0] = ((paddr >> 12) << 10) | pte_flags;

    return 0;
}

static int riscv32_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    if (paddr) *paddr = (l0_table->entries[vpn0] >> 10) << 12;
    if (flags) *flags = riscv_to_flags(l0_table->entries[vpn0] & ~RISCV32_PAGE_MASK);

    return 0;
}

static const hal_pt_caps_t riscv32_pt_caps = {
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
    .supports_ad_bits = true,
    .supports_large_2m = false,
    .supports_large_1g = false,
    .supports_device_memtype = true,
    .supports_writecombine = false,
    .requires_bbm = false,
    .supports_cow_softbit = false,
    .supports_linear_physmap = false,
};

extern hal_tlb_ops_t riscv32_hal_tlb_ops;

hal_pt_ops_t riscv32_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MMU,
    .caps                  = &riscv32_pt_caps,
    .create_address_space  = riscv32_create_address_space,
    .destroy_address_space = riscv32_destroy_address_space,
    .map_page              = riscv32_map_page,
    .unmap_page            = riscv32_unmap_page,
    .protect_page          = riscv32_protect_page,
    .query_page            = riscv32_query_page,
    .map_range             = NULL,
    .unmap_range           = NULL,
    .protect_range         = NULL,
    .query_mapping         = NULL,
};

void arch_hal_pt_init(void) {
    hal_pt_register_ops(&riscv32_hal_pt_ops, &riscv32_hal_tlb_ops);
}

static void riscv32_tlb_flush_page_local(virt_addr_t vaddr) {
    asm volatile(
        "sfence.vma %0\n"
        :: "r"(vaddr) : "memory"
    );
}

static void riscv32_tlb_flush_all_local(void) {
    asm volatile("sfence.vma\n" ::: "memory");
}

static void riscv32_tlb_flush_asid_local(uint16_t asid) {
    asm volatile(
        "sfence.vma zero, %0\n"
        :: "r"(asid) : "memory"
    );
}


static const hal_tlb_caps_t riscv32_tlb_caps = {
    .supports_page_flush = true,
    .supports_range_flush = false,
    .supports_aspace_flush = true,
    .supports_all_flush = true,
    .supports_remote_targeted_flush = false,
    .supports_broadcast_flush = false,
    .supports_asid_selective_flush = true,
    .supports_lazy_generation_model = false,
};

hal_tlb_ops_t riscv32_hal_tlb_ops = {
    .caps                 = &riscv32_tlb_caps,
    .flush_page_local      = riscv32_tlb_flush_page_local,
    .flush_range_local     = NULL,
    .flush_all_local       = riscv32_tlb_flush_all_local,
    .flush_asid_local      = riscv32_tlb_flush_asid_local,
    .flush_page_remote     = NULL,
    .flush_range_remote    = NULL,
    .flush_all_remote      = NULL,
    .flush_page_broadcast  = NULL,
    .flush_range_broadcast = NULL,
    .flush_all_broadcast   = NULL,
};

// Override symbol to avoid redefinition if built alongside others
#if defined(__riscv) && __riscv_xlen == 32
const hal_translate_ops_t* hal_translate_ops(void) {
    return &riscv32_translate_ops;
}
#endif

static phys_addr_t riscv32_mpa_make_table(uint32_t level) {
    (void)level;
    return mm_alloc_page(NUMA_NODE_ANY);
}

static int riscv32_mpa_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return riscv32_map_page(root_pt, vaddr, paddr, flags);
}

static int riscv32_mpa_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    return riscv32_unmap_page(root_pt, vaddr, unmapped_paddr);
}

static void riscv32_mpa_set_root(phys_addr_t root) {
    uint32_t satp = (1u << 31) | ((uint32_t)(root >> 12) & 0x003FFFFFu); // Sv32
    __asm__ volatile("csrw satp, %0" :: "r"(satp) : "memory");
    __asm__ volatile("sfence.vma zero, zero" ::: "memory");
}

static void riscv32_mpa_flush_tlb_local(virt_addr_t vaddr, uint16_t asid) {
    (void)asid;
    riscv32_tlb_flush_page_local(vaddr);
}

static phys_addr_t riscv32_mpa_get_root(void) {
    uint32_t satp;
    __asm__ volatile("csrr %0, satp" : "=r"(satp));
    return ((phys_addr_t)(satp & 0x003FFFFFu)) << 12;
}

mem_protect_ops_t riscv32_mem_protect_ops = {
    .supported_caps = MPA_CAP_VIRT | MPA_CAP_EXEC_PERM | MPA_CAP_WRITE | MPA_CAP_USER | MPA_CAP_DEVICE,
    .cpu_ops = {
        .make_table = riscv32_mpa_make_table,
        .map_page = riscv32_mpa_map_page,
        .unmap_page = riscv32_mpa_unmap_page,
        .set_root = riscv32_mpa_set_root,
        .flush_tlb_local = riscv32_mpa_flush_tlb_local,
        .get_root = riscv32_mpa_get_root,
    },
    .iommu_ops = {
        .probe = NULL,
    }
};

mem_protect_ops_t *active_mem_protect = &riscv32_mem_protect_ops;
