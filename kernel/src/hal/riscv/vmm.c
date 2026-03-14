#include "../../../include/hal/vmm.h"
#include "../../../include/hal/mmu_ops.h"
#include "../../../include/numa.h"

// Define direct map macros for early initialization.
// We are mimicking the logic used in the generic VMM for mapping.
#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

// RISC-V Sv39 Page Table Entry Flags
#define RISCV_PTE_V (1ULL << 0) // Valid
#define RISCV_PTE_R (1ULL << 1) // Read
#define RISCV_PTE_W (1ULL << 2) // Write
#define RISCV_PTE_X (1ULL << 3) // Execute
#define RISCV_PTE_U (1ULL << 4) // User

typedef struct {
    uint64_t entries[512];
} pte_table_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & ~(virt_addr_t)(PAGE_SIZE - 1U);
}

// Helper to convert architecture independent flags to RISC-V specific flags
static uint64_t convert_flags_to_riscv(uint32_t flags) {
    uint64_t pte_flags = RISCV_PTE_V | RISCV_PTE_R; // Always readable

    if ((flags & CAP_RIGHT_WRITE) != 0) pte_flags |= RISCV_PTE_W;
    if ((flags & PAGE_USER) != 0) pte_flags |= RISCV_PTE_U;

    // We can add Execute based on standard rights, but for now we simplify.

    // Keep internal PAGE_COW flag or similar in higher bits if needed,
    // though RISC-V uses bits 8-9 for RSW (Reserved for Software).
    if ((flags & PAGE_COW) != 0) {
        pte_flags |= (1ULL << 8); // Store COW in RSW
    }

    return pte_flags;
}

static uint32_t convert_riscv_to_flags(uint64_t pte_flags) {
    uint32_t flags = 0;
    if ((pte_flags & RISCV_PTE_W) != 0) flags |= CAP_RIGHT_WRITE;
    if ((pte_flags & RISCV_PTE_U) != 0) flags |= PAGE_USER;
    if ((pte_flags & (1ULL << 8)) != 0) flags |= PAGE_COW;

    // For CAP_RIGHT_READ we typically assume read access for mapped pages
    return flags;
}

phys_addr_t hal_vmm_init_root(void) {
    phys_addr_t root_dir_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (root_dir_phys == 0U) {
        return 0;
    }

    pte_table_t* l2_table = (pte_table_t*)P2V(root_dir_phys);
    for (int i = 0; i < 512; i++) {
        l2_table->entries[i] = 0;
    }

    return root_dir_phys;
}

static uint64_t hal_get_satp(void) {
    uint64_t satp;
    __asm__ volatile("csrr %0, satp" : "=r"(satp));
    // Extracts physical address from satp (assuming Sv39, ppn is lower 44 bits)
    return (satp & ((1ULL << 44) - 1)) << 12;
}

static void hal_sfence_vma(virt_addr_t vaddr) {
    __asm__ volatile("sfence.vma %0" :: "r"(vaddr) : "memory");
}

int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U || paddr == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    // Sv39 uses 3 levels of page tables: VPN[2] (L2), VPN[1] (L1), VPN[0] (L0)
    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pte_table_t* l2_table = (pte_table_t*)P2V(root_table);

    if ((l2_table->entries[vpn2] & RISCV_PTE_V) == 0) {
        phys_addr_t new_l1 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l1) return -2;
        pte_table_t* l1_ptr = (pte_table_t*)P2V(new_l1);
        for(int i=0; i<512; i++) l1_ptr->entries[i] = 0;
        l2_table->entries[vpn2] = ((new_l1 >> 12) << 10) | RISCV_PTE_V | RISCV_PTE_U; // Directory entry has no R/W/X
    }

    pte_table_t* l1_table = (pte_table_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);

    if ((l1_table->entries[vpn1] & RISCV_PTE_V) == 0) {
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pte_table_t* l0_ptr = (pte_table_t*)P2V(new_l0);
        for(int i=0; i<512; i++) l0_ptr->entries[i] = 0;
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | RISCV_PTE_V | RISCV_PTE_U;
    }

    pte_table_t* l0_table = (pte_table_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    // Create final PTE for L0 (page mapping)
    uint64_t pte_flags = convert_flags_to_riscv(flags);
    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

    if (root_table == hal_get_satp()) {
        hal_sfence_vma(aligned_vaddr);
    }

    return 0;
}

// -----------------------------------------------------------------------------
// mmu_ops_t Wrapper implementations
// -----------------------------------------------------------------------------

static phys_addr_t riscv_mmu_create_table(void) {
    return hal_vmm_init_root();
}

static void riscv_mmu_destroy_table_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pte_table_t* pt = (pte_table_t*)P2V(table);
        for (int i = 0; i < (level == 2 ? 256 : 512); i++) { // Skip top half on l2 (Sv39)
            if (pt->entries[i] & RISCV_PTE_V) {
                if ((pt->entries[i] & (RISCV_PTE_R | RISCV_PTE_W | RISCV_PTE_X)) != 0) {
                    continue; // Leaf page, don't recurse
                }
                riscv_mmu_destroy_table_recursive((pt->entries[i] >> 10) << 12, level - 1);
            }
        }
    }
    mm_free_page(table);
}

static void riscv_mmu_destroy_table(phys_addr_t root) {
    if (root) {
        riscv_mmu_destroy_table_recursive(root, 2); // Sv39 root is L2
    }
}

static phys_addr_t riscv_mmu_clone_kernel(phys_addr_t kernel_root) {
    return hal_vmm_setup_address_space(kernel_root);
}

#define RISCV_PBMT_NC (1ULL << 61) // Non-cacheable (if Svpbmt is supported)
#define RISCV_PBMT_IO (2ULL << 61) // IO/Device (if Svpbmt is supported)

// Translate generic mmu_flags_t to RISC-V specific VMM flags
static uint32_t flags_to_riscv(mmu_flags_t f) {
    uint32_t flags = 0;
    if (f & MMU_WRITE)   flags |= CAP_RIGHT_WRITE;
    if (f & MMU_USER)    flags |= PAGE_USER;
    if (f & MMU_COW)     flags |= PAGE_COW;
    // We store DEVICE/NOCACHE in upper bits of flags to pass to convert_flags_to_riscv
    if (f & MMU_DEVICE)  flags |= (1U << 30); // Use a high bit to signal device mem
    if (f & MMU_NOCACHE) flags |= (1U << 31); // Use a high bit to signal nocache
    return flags;
}

// Translate RISC-V specific VMM flags to generic mmu_flags_t
static mmu_flags_t riscv_to_flags(uint32_t flags) {
    mmu_flags_t f = MMU_READ;
    if (flags & CAP_RIGHT_WRITE) f |= MMU_WRITE;
    if (flags & PAGE_USER)       f |= MMU_USER;
    if (flags & PAGE_COW)        f |= MMU_COW;
    return f;
}

static int riscv_mmu_map(phys_addr_t root, virt_addr_t virt, phys_addr_t phys,
                         size_t size, mmu_flags_t flags) {
    size_t mapped = 0;
    while (mapped < size) {
        int ret = hal_vmm_map_page(root, virt + mapped, phys + mapped, flags_to_riscv(flags));
        if (ret < 0) return ret;
        mapped += PAGE_SIZE;
    }
    return 0;
}

static int riscv_mmu_unmap(phys_addr_t root, virt_addr_t virt, size_t size, phys_addr_t *unmapped_phys) {
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

static int riscv_mmu_protect(phys_addr_t root, virt_addr_t virt, size_t size, mmu_flags_t new_flags) {
    size_t updated = 0;
    while (updated < size) {
        phys_addr_t phys = 0;
        uint32_t old_flags = 0;
        int ret = hal_vmm_get_mapping(root, virt + updated, &phys, &old_flags);
        if (ret == 0 && phys != 0) {
            hal_vmm_update_mapping(root, virt + updated, phys, flags_to_riscv(new_flags));
        }
        updated += PAGE_SIZE;
    }
    return 0;
}

static int riscv_mmu_query(phys_addr_t root, virt_addr_t virt, phys_addr_t *phys_out, mmu_flags_t *flags_out) {
    uint32_t riscv_flags = 0;
    int ret = hal_vmm_get_mapping(root, virt, phys_out, &riscv_flags);
    if (ret == 0 && flags_out) {
        *flags_out = riscv_to_flags(riscv_flags);
    }
    return ret;
}

static void riscv_mmu_activate(phys_addr_t root) {
    // Mode 8 is Sv39
    uintptr_t ppn = (uintptr_t)root >> 12;
    uintptr_t satp = (8ULL << 60) | ppn;
    asm volatile(
        "csrw satp, %0\n"
        "sfence.vma\n"
        :: "r"(satp) : "memory"
    );
}

static void riscv_mmu_deactivate(void) {
    // Mode 0 is BARE
    asm volatile(
        "csrw satp, zero\n"
        "sfence.vma\n"
        ::: "memory"
    );
}

static void riscv_mmu_tlb_flush_page(virt_addr_t virt) {
    asm volatile(
        "sfence.vma %0\n"
        :: "r"(virt) : "memory"
    );
}

static void riscv_mmu_tlb_flush_all(void) {
    asm volatile("sfence.vma\n" ::: "memory");
}

static void riscv_mmu_tlb_flush_asid(uint16_t asid) {
    asm volatile(
        "sfence.vma zero, %0\n"
        :: "r"(asid) : "memory"
    );
}

static size_t riscv_huge_pages[] = {0x200000, 0x40000000, 0}; // 2MB, 1GB (for Sv39)

mmu_ops_t riscv64_mmu_ops = {
    .create_table     = riscv_mmu_create_table,
    .destroy_table    = riscv_mmu_destroy_table,
    .clone_kernel     = riscv_mmu_clone_kernel,
    .map              = riscv_mmu_map,
    .unmap            = riscv_mmu_unmap,
    .protect          = riscv_mmu_protect,
    .query            = riscv_mmu_query,
    .activate         = riscv_mmu_activate,
    .deactivate       = riscv_mmu_deactivate,
    .tlb_flush_page   = riscv_mmu_tlb_flush_page,
    .tlb_flush_all    = riscv_mmu_tlb_flush_all,
    .tlb_flush_asid   = riscv_mmu_tlb_flush_asid,

    .page_size        = 4096,
    .huge_page_sizes  = riscv_huge_pages,
    .levels           = 3, // Sv39
    .has_nx           = true, // R/W/X bits are independent
    .asid_bits        = 16, // Up to 16 bits in ASID field of satp
    .has_user_kernel_split = false, // RISC-V uses shared address space
};

void riscv_mmu_detect(mmu_ops_t *ops) {
    // Stub for runtime detection (e.g. probing satp for Sv48/Sv57)
    (void)ops;
}

int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr) {
    if (root_table == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pte_table_t* l2_table = (pte_table_t*)P2V(root_table);
    if ((l2_table->entries[vpn2] & RISCV_PTE_V) == 0) return -2;

    pte_table_t* l1_table = (pte_table_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PTE_V) == 0) return -2;

    pte_table_t* l0_table = (pte_table_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    if (unmapped_paddr) {
        *unmapped_paddr = (l0_table->entries[vpn0] >> 10) << 12;
    }

    l0_table->entries[vpn0] = 0;

    if (root_table == hal_get_satp()) {
        hal_sfence_vma(aligned_vaddr);
    }

    return 0;
}

phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pte_table_t* l2_table = (pte_table_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        l2_table->entries[i] = 0;
    }

    if (kernel_root_table != 0U) {
        pte_table_t* kernel_l2 = (pte_table_t*)P2V(kernel_root_table);
        // Assuming kernel space is mapped in the higher half.
        // We can copy the top entries (e.g., 256-511) to share kernel mappings.
        // On typical Sv39 setup, kernel mappings are in the upper 256 entries.
        for(int i = 256; i < 512; i++) {
            l2_table->entries[i] = kernel_l2->entries[i];
        }
    }

    return root;
}

int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    if (root_table == 0U) return -1;

    uint64_t vpn2 = (vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (vaddr >> 12) & 0x1FF;

    pte_table_t* l2_table = (pte_table_t*)P2V(root_table);
    if ((l2_table->entries[vpn2] & RISCV_PTE_V) == 0) return -2;

    pte_table_t* l1_table = (pte_table_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PTE_V) == 0) return -2;

    pte_table_t* l0_table = (pte_table_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    uint64_t old_entry = l0_table->entries[vpn0];
    if ((old_entry & RISCV_PTE_V) == 0) return -2;

    if (paddr) *paddr = (old_entry >> 10) << 12;
    if (flags) *flags = convert_riscv_to_flags(old_entry);

    return 0;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t vpn2 = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t vpn1 = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t vpn0 = (aligned_vaddr >> 12) & 0x1FF;

    pte_table_t* l2_table = (pte_table_t*)P2V(root_table);
    if ((l2_table->entries[vpn2] & RISCV_PTE_V) == 0) return -2;

    pte_table_t* l1_table = (pte_table_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);
    if ((l1_table->entries[vpn1] & RISCV_PTE_V) == 0) return -2;

    pte_table_t* l0_table = (pte_table_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    uint64_t pte_flags = convert_flags_to_riscv(flags);
    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

    if (root_table == hal_get_satp()) {
        hal_sfence_vma(aligned_vaddr);
    }

    return 0;
}
