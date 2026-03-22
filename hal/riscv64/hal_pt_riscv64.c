#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm.h"
#include "../../kernel/include/numa.h"
#include "../../kernel/include/mm/physmap.h"
#include <stdbool.h>

// Direct-Map Subsystem Configuration
// For RISC-V Sv39, the kernel virtual offset (high-half map)
const virt_addr_t g_kernel_virt_offset = 0xFFFFFFC000000000ULL;
const size_t g_kernel_physmap_size = 0x4000000000ULL; // 256GB

// RISC-V Sv39 Page Table Entry bits
#define RISCV_PT_V (1ULL << 0) // Valid
#define RISCV_PT_R (1ULL << 1) // Read
#define RISCV_PT_W (1ULL << 2) // Write
#define RISCV_PT_X (1ULL << 3) // Execute
#define RISCV_PT_U (1ULL << 4) // User
#define RISCV_PT_G (1ULL << 5) // Global
#define RISCV_PT_A (1ULL << 6) // Accessed
#define RISCV_PT_D (1ULL << 7) // Dirty

// Reserved for software
#define RISCV_PT_RSW (3ULL << 8)

// Svpbmt bits (PBMT) for memory types
#define RISCV_PT_PBMT_NC (1ULL << 61) // Non-cacheable
#define RISCV_PT_PBMT_IO (2ULL << 61) // Memory-mapped I/O

#define RISCV_PAGE_MASK (~0xFFFULL)

// Arch-private raw descriptor
typedef uint64_t pte_raw_t;

typedef struct {
    pte_raw_t entries[512];
} pt_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & RISCV_PAGE_MASK;
}

static bool table_empty(pt_t* table) {
    for (size_t i = 0; i < 512; i++) {
        if (table->entries[i] & RISCV_PT_V) {
            return false;
        }
    }
    return true;
}

static uint64_t flags_to_riscv(uint32_t flags) {
    uint64_t pte_flags = RISCV_PT_V | RISCV_PT_A | RISCV_PT_D; // Pre-set A/D bits for simplicity

    if (flags & HAL_PT_FLAG_READ)  pte_flags |= RISCV_PT_R;
    if (flags & HAL_PT_FLAG_WRITE) pte_flags |= RISCV_PT_W;
    if (flags & HAL_PT_FLAG_EXEC)  pte_flags |= RISCV_PT_X;
    if (flags & HAL_PT_FLAG_USER)  pte_flags |= RISCV_PT_U;
    if (flags & HAL_PT_FLAG_GLOBAL) pte_flags |= RISCV_PT_G;

    // Device/Nocache attributes via Svpbmt extension
    if (flags & HAL_PT_FLAG_DEVICE) {
        pte_flags |= RISCV_PT_PBMT_IO;
    } else if (flags & HAL_PT_FLAG_NOCACHE) {
        pte_flags |= RISCV_PT_PBMT_NC;
    }

    return pte_flags;
}

static uint32_t riscv_to_flags(uint64_t pte_flags) {
    uint32_t flags = 0;

    if (pte_flags & RISCV_PT_R) flags |= HAL_PT_FLAG_READ;
    if (pte_flags & RISCV_PT_W) flags |= HAL_PT_FLAG_WRITE;
    if (pte_flags & RISCV_PT_X) flags |= HAL_PT_FLAG_EXEC;
    if (pte_flags & RISCV_PT_U) flags |= HAL_PT_FLAG_USER;
    if (pte_flags & RISCV_PT_G) flags |= HAL_PT_FLAG_GLOBAL;

    uint64_t pbmt = pte_flags & (3ULL << 61);
    if (pbmt == RISCV_PT_PBMT_IO) {
        flags |= HAL_PT_FLAG_DEVICE;
    } else if (pbmt == RISCV_PT_PBMT_NC) {
        flags |= HAL_PT_FLAG_NOCACHE;
    }

    return flags;
}

static phys_addr_t riscv64_pt_create_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pt_t* l2_table = (pt_t*)physmap_phys_to_virt(root);
    for (int i = 0; i < 512; i++) {
        l2_table->entries[i] = 0;
    }

    // In Sv39, the top half of the L2 table is typically used for kernel space.
    if (kernel_root_table != 0U) {
        pt_t* kernel_l2 = (pt_t*)physmap_phys_to_virt(kernel_root_table);
        for(int i = 256; i < 512; i++) {
            l2_table->entries[i] = kernel_l2->entries[i];
        }
    }

    return root;
}

static void riscv64_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pt_t* pt = (pt_t*)physmap_phys_to_virt(table);
        int max_idx = (level == 2) ? 256 : 512; // Free only bottom half (User) of L2
        for (int i = 0; i < max_idx; i++) {
            if (pt->entries[i] & RISCV_PT_V) {
                // If any of R/W/X are set, it's a leaf block descriptor, don't recurse
                if ((pt->entries[i] & (RISCV_PT_R | RISCV_PT_W | RISCV_PT_X)) == 0) {
                    riscv64_pt_destroy_recursive((pt->entries[i] >> 10) << 12, level - 1);
                }
            }
        }
    }
    mm_free_page(table);
}

static void riscv64_pt_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        riscv64_pt_destroy_recursive(root_pt, 2);
    }
}

static int riscv64_pt_map_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)physmap_phys_to_virt(root_pt);

    uint64_t table_flags = RISCV_PT_V; // Intermediate tables only need V bit

    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) {
        phys_addr_t new_l1 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l1) return -2;
        pt_t* l1_ptr = (pt_t*)physmap_phys_to_virt(new_l1);
        for(int i=0; i<512; i++) l1_ptr->entries[i] = 0;
        l2_table->entries[vpn2] = ((new_l1 >> 12) << 10) | table_flags;
    }

    pt_t* l1_table = (pt_t*)physmap_phys_to_virt((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) {
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pt_t* l0_ptr = (pt_t*)physmap_phys_to_virt(new_l0);
        for(int i=0; i<512; i++) l0_ptr->entries[i] = 0;
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | table_flags;
    }

    pt_t* l0_table = (pt_t*)physmap_phys_to_virt((l1_table->entries[vpn1] >> 10) << 12);

    uint64_t pte_flags = flags_to_riscv(flags);
    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

    return 0;
}

static int riscv64_pt_unmap_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)physmap_phys_to_virt(root_pt);
    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) return -2;
    pt_t* l1_table = (pt_t*)physmap_phys_to_virt((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)physmap_phys_to_virt((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV_PT_V) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = (l0_table->entries[vpn0] >> 10) << 12;
    }

    l0_table->entries[vpn0] = 0;

    if (table_empty(l0_table)) {
        mm_free_page((l1_table->entries[vpn1] >> 10) << 12);
        l1_table->entries[vpn1] = 0;
        if (table_empty(l1_table)) {
            mm_free_page((l2_table->entries[vpn2] >> 10) << 12);
            l2_table->entries[vpn2] = 0;
        }
    }

    return 0;
}

static int riscv64_pt_protect_4k(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)physmap_phys_to_virt(root_pt);
    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) return -2;
    pt_t* l1_table = (pt_t*)physmap_phys_to_virt((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)physmap_phys_to_virt((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV_PT_V) == 0) return -2;

    uint64_t paddr = (l0_table->entries[vpn0] >> 10) << 12;
    uint64_t pte_flags = flags_to_riscv(new_flags);

    l0_table->entries[vpn0] = ((paddr >> 12) << 10) | pte_flags;

    return 0;
}

static int riscv64_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)physmap_phys_to_virt(root_pt);
    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) return -2;
    pt_t* l1_table = (pt_t*)physmap_phys_to_virt((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)physmap_phys_to_virt((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV_PT_V) == 0) return -2;

    if (paddr) *paddr = (l0_table->entries[vpn0] >> 10) << 12;
    if (flags) *flags = riscv_to_flags(l0_table->entries[vpn0] & ~RISCV_PAGE_MASK);

    return 0;
}

static int riscv64_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    size_t done = 0;
    while (done < size) {
        int rc = riscv64_pt_map_4k(root_pt, vaddr + done, paddr + done, flags);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int riscv64_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    size_t done = 0;
    while (done < size) {
        int rc = riscv64_pt_unmap_4k(root_pt, vaddr + done, NULL);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int riscv64_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    size_t done = 0;
    while (done < size) {
        int rc = riscv64_pt_protect_4k(root_pt, vaddr + done, new_flags);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int riscv64_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return riscv64_pt_map_range(root_pt, vaddr, paddr, PAGE_SIZE, flags);
}

static int riscv64_pt_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (unmapped_paddr) {
        (void)riscv64_pt_query_page(root_pt, vaddr, unmapped_paddr, NULL);
    }
    return riscv64_pt_unmap_range(root_pt, vaddr, PAGE_SIZE);
}

static int riscv64_pt_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    return riscv64_pt_protect_range(root_pt, vaddr, PAGE_SIZE, new_flags);
}

static int riscv64_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
    int rc = riscv64_pt_query_page(root_pt, vaddr, paddr, flags);
    if (rc != 0) return rc;
    if (mapped_size) *mapped_size = PAGE_SIZE;
    return 0;
}

static translate_backend_kind_t riscv64_backend_type(void) { return TRANSLATE_BACKEND_MMU; }
static translate_exec_class_t riscv64_exec_class(void) { return TRANSLATE_EXEC_MMU_FULL; }

static void* riscv64_phys_to_virt(phys_addr_t phys) {
    uint64_t satp;
    asm volatile("csrr %0, satp" : "=r"(satp));
    if ((satp >> 60) == 0) {
        return (void*)phys; // MMU is disabled (Bare mode), return identity mapping
    }
    return (void*)(phys + g_kernel_virt_offset);
}

static phys_addr_t riscv64_virt_to_phys(const void* virt) {
    uint64_t satp;
    asm volatile("csrr %0, satp" : "=r"(satp));
    if ((satp >> 60) == 0) {
        return (phys_addr_t)virt; // MMU is disabled (Bare mode), return identity mapping
    }
    return (phys_addr_t)((uintptr_t)virt - g_kernel_virt_offset);
}

static bool riscv64_has_linear_physmap(void) { return true; }
static phys_addr_t riscv64_linear_physmap_base(void) { return g_kernel_virt_offset; }
static phys_addr_t riscv64_linear_physmap_limit(void) { return g_kernel_virt_offset + g_kernel_physmap_size; }

static const hal_translate_ops_t riscv64_translate_ops = {
    .backend_type = riscv64_backend_type,
    .exec_class = riscv64_exec_class,
    .phys_to_virt = riscv64_phys_to_virt,
    .virt_to_phys = riscv64_virt_to_phys,
    .has_linear_physmap = riscv64_has_linear_physmap,
    .linear_physmap_base = riscv64_linear_physmap_base,
    .linear_physmap_limit = riscv64_linear_physmap_limit,
};

const hal_translate_ops_t* hal_translate_ops(void) {
    return &riscv64_translate_ops;
}

static const hal_pt_caps_t riscv64_pt_caps = {
    .backend_kind = TRANSLATE_BACKEND_MMU,
    .exec_class = TRANSLATE_EXEC_MMU_FULL,
    .supports_sparse_vm = true,
    .supports_demand_fault = true,
    .supports_protect = true,
    .supports_query = true,
    .supports_range_map = true,
    .supports_range_unmap = true,
    .supports_range_protect = true,
    .supports_asid = true,
    .supports_pcid = false,
    .supports_global = true,
    .supports_nx_or_xn = false,
    .supports_ad_bits = true,
    .supports_large_2m = true,
    .supports_large_1g = false,
    .supports_device_memtype = true,
    .supports_writecombine = false,
    .requires_bbm = false,
    .supports_cow_softbit = true,
    .supports_linear_physmap = true,
};

hal_pt_ops_t riscv64_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MMU,
    .caps                  = &riscv64_pt_caps,
    .create_address_space  = riscv64_pt_create_address_space,
    .destroy_address_space = riscv64_pt_destroy_address_space,
    .map_page              = riscv64_pt_map_page,
    .unmap_page            = riscv64_pt_unmap_page,
    .protect_page          = riscv64_pt_protect_page,
    .query_page            = riscv64_pt_query_page,
    .map_range             = riscv64_pt_map_range,
    .unmap_range           = riscv64_pt_unmap_range,
    .protect_range         = riscv64_pt_protect_range,
    .query_mapping         = riscv64_pt_query_mapping,
};

static void riscv64_tlb_flush_page_local(virt_addr_t vaddr) {
    asm volatile(
        "sfence.vma %0\n"
        :: "r"(vaddr) : "memory"
    );
}

static void riscv64_tlb_flush_all_local(void) {
    asm volatile("sfence.vma\n" ::: "memory");
}

static void riscv64_tlb_flush_asid_local(uint16_t asid) {
    asm volatile(
        "sfence.vma zero, %0\n"
        :: "r"(asid) : "memory"
    );
}

static void riscv64_tlb_flush_page_remote(uint16_t target_core, uint16_t asid, virt_addr_t vaddr) {
    (void)target_core;
    (void)asid;
    (void)vaddr;
    // Handled by URPC or architectural IPI in RISC-V
}

static void riscv64_tlb_flush_all_remote(uint16_t target_core, uint16_t asid) {
    (void)target_core;
    (void)asid;
}

static void riscv64_tlb_flush_page_broadcast(uint16_t asid, virt_addr_t vaddr) {
    (void)asid;
    (void)vaddr;
    // Not directly supported in hardware without IPIs in RISC-V generally
}

static void riscv64_tlb_flush_all_broadcast(uint16_t asid) {
    (void)asid;
}

hal_tlb_ops_t riscv64_hal_tlb_ops = {
    .caps                 = &(const hal_tlb_caps_t){
        .supports_page_flush = true,
        .supports_range_flush = false,
        .supports_aspace_flush = true,
        .supports_all_flush = true,
        .supports_remote_targeted_flush = false,
        .supports_broadcast_flush = false,
        .supports_asid_selective_flush = true,
        .supports_lazy_generation_model = false,
    },
    .flush_page_local      = riscv64_tlb_flush_page_local,
    .flush_all_local       = riscv64_tlb_flush_all_local,
    .flush_asid_local      = riscv64_tlb_flush_asid_local,
    .flush_page_remote     = riscv64_tlb_flush_page_remote,
    .flush_all_remote      = riscv64_tlb_flush_all_remote,
    .flush_page_broadcast  = riscv64_tlb_flush_page_broadcast,
    .flush_all_broadcast   = riscv64_tlb_flush_all_broadcast,
};
