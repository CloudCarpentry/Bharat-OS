#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm.h"
#include "../../../include/numa.h"

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

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

typedef struct {
    uint64_t entries[512];
} pt_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & RISCV_PAGE_MASK;
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

phys_addr_t riscv64_pt_create_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pt_t* l2_table = (pt_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        l2_table->entries[i] = 0;
    }

    // In Sv39, the top half of the L2 table is typically used for kernel space.
    if (kernel_root_table != 0U) {
        pt_t* kernel_l2 = (pt_t*)P2V(kernel_root_table);
        for(int i = 256; i < 512; i++) {
            l2_table->entries[i] = kernel_l2->entries[i];
        }
    }

    return root;
}

static void riscv64_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pt_t* pt = (pt_t*)P2V(table);
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

void riscv64_pt_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        riscv64_pt_destroy_recursive(root_pt, 2);
    }
}

int riscv64_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)P2V(root_pt);

    uint64_t table_flags = RISCV_PT_V; // Intermediate tables only need V bit

    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) {
        phys_addr_t new_l1 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l1) return -2;
        pt_t* l1_ptr = (pt_t*)P2V(new_l1);
        for(int i=0; i<512; i++) l1_ptr->entries[i] = 0;
        l2_table->entries[vpn2] = ((new_l1 >> 12) << 10) | table_flags;
    }

    pt_t* l1_table = (pt_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) {
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pt_t* l0_ptr = (pt_t*)P2V(new_l0);
        for(int i=0; i<512; i++) l0_ptr->entries[i] = 0;
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | table_flags;
    }

    pt_t* l0_table = (pt_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    uint64_t pte_flags = flags_to_riscv(flags);
    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

    return 0;
}

int riscv64_pt_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)P2V(root_pt);
    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) return -2;
    pt_t* l1_table = (pt_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV_PT_V) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = (l0_table->entries[vpn0] >> 10) << 12;
    }

    l0_table->entries[vpn0] = 0;

    return 0;
}

int riscv64_pt_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)P2V(root_pt);
    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) return -2;
    pt_t* l1_table = (pt_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV_PT_V) == 0) return -2;

    uint64_t paddr = (l0_table->entries[vpn0] >> 10) << 12;
    uint64_t pte_flags = flags_to_riscv(new_flags);

    l0_table->entries[vpn0] = ((paddr >> 12) << 10) | pte_flags;

    return 0;
}

int riscv64_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* l2_table = (pt_t*)P2V(root_pt);
    if ((l2_table->entries[vpn2] & RISCV_PT_V) == 0) return -2;
    pt_t* l1_table = (pt_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV_PT_V) == 0) return -2;

    if (paddr) *paddr = (l0_table->entries[vpn0] >> 10) << 12;
    if (flags) *flags = riscv_to_flags(l0_table->entries[vpn0] & ~RISCV_PAGE_MASK);

    return 0;
}

hal_pt_ops_t riscv64_hal_pt_ops = {
    .create_address_space  = riscv64_pt_create_address_space,
    .destroy_address_space = riscv64_pt_destroy_address_space,
    .map_page              = riscv64_pt_map_page,
    .unmap_page            = riscv64_pt_unmap_page,
    .protect_page          = riscv64_pt_protect_page,
    .query_page            = riscv64_pt_query_page,
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
    .flush_page_local      = riscv64_tlb_flush_page_local,
    .flush_all_local       = riscv64_tlb_flush_all_local,
    .flush_asid_local      = riscv64_tlb_flush_asid_local,
    .flush_page_remote     = riscv64_tlb_flush_page_remote,
    .flush_all_remote      = riscv64_tlb_flush_all_remote,
    .flush_page_broadcast  = riscv64_tlb_flush_page_broadcast,
    .flush_all_broadcast   = riscv64_tlb_flush_all_broadcast,
};
