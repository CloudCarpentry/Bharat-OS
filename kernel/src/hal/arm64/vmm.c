#include "../../../include/hal/vmm.h"
#include "../../../include/numa.h"

// Define direct map macros for early initialization.
// Mimicking the logic used in generic VMM for identity map base.
#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

// ARM64 Translation Table Descriptor Types & Flags (VMSAv8-64)
#define ARM64_MMU_DESCRIPTOR_INVALID (0ULL)
#define ARM64_MMU_DESCRIPTOR_BLOCK   (1ULL) // L1/L2 block
#define ARM64_MMU_DESCRIPTOR_TABLE   (3ULL) // L0/L1/L2 table pointer
#define ARM64_MMU_DESCRIPTOR_PAGE    (3ULL) // L3 page

#define ARM64_MMU_FLAG_VALID         (1ULL << 0)
#define ARM64_MMU_FLAG_AF            (1ULL << 10) // Access flag

// Default memory attributes (simplification for memory mapping)
#define ARM64_MMU_ATTR_NORMAL_MEM    (0ULL << 2)  // Index to MAIR
#define ARM64_MMU_SH_INNER           (3ULL << 8)  // Inner shareable

// AP bits
#define ARM64_MMU_AP_RW_EL1          (0ULL << 6) // EL1 R/W
#define ARM64_MMU_AP_RO_EL1          (2ULL << 6) // EL1 R/O
#define ARM64_MMU_AP_RW_EL0          (1ULL << 6) // EL0 R/W, EL1 R/W
#define ARM64_MMU_AP_RO_EL0          (3ULL << 6) // EL0 R/O, EL1 R/O

// XN bit (Execute Never)
#define ARM64_MMU_UXN                (1ULL << 54)
#define ARM64_MMU_PXN                (1ULL << 53)

typedef struct {
    uint64_t entries[512];
} pt_t, pmd_t, pud_t, pgd_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & ~(virt_addr_t)(PAGE_SIZE - 1U);
}

// Convert architecture independent flags to ARM64 specific flags
static uint64_t convert_flags_to_arm64(uint32_t flags) {
    uint64_t mmu_flags = ARM64_MMU_FLAG_AF | ARM64_MMU_ATTR_NORMAL_MEM | ARM64_MMU_SH_INNER;

    // Permissions mapping
    if ((flags & PAGE_USER) != 0) {
        if ((flags & CAP_RIGHT_WRITE) != 0) {
            mmu_flags |= ARM64_MMU_AP_RW_EL0;
        } else {
            mmu_flags |= ARM64_MMU_AP_RO_EL0;
        }
        mmu_flags |= ARM64_MMU_PXN; // User pages are usually PXN
    } else {
        if ((flags & CAP_RIGHT_WRITE) != 0) {
            mmu_flags |= ARM64_MMU_AP_RW_EL1;
        } else {
            mmu_flags |= ARM64_MMU_AP_RO_EL1;
        }
        mmu_flags |= ARM64_MMU_UXN; // Kernel pages UXN
    }

    // COW flag can be stored in software available bits (e.g., bits 55-58)
    if ((flags & PAGE_COW) != 0) {
        mmu_flags |= (1ULL << 55);
    }

    return mmu_flags;
}

static uint32_t convert_arm64_to_flags(uint64_t mmu_flags) {
    uint32_t flags = 0;

    uint64_t ap = mmu_flags & (3ULL << 6);
    if (ap == ARM64_MMU_AP_RW_EL0 || ap == ARM64_MMU_AP_RO_EL0) {
        flags |= PAGE_USER;
    }

    if (ap == ARM64_MMU_AP_RW_EL0 || ap == ARM64_MMU_AP_RW_EL1) {
        flags |= CAP_RIGHT_WRITE;
    }

    if ((mmu_flags & (1ULL << 55)) != 0) {
        flags |= PAGE_COW;
    }

    return flags;
}

phys_addr_t hal_vmm_init_root(void) {
    phys_addr_t root_dir_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (root_dir_phys == 0U) {
        return 0;
    }

    pgd_t* pgd = (pgd_t*)P2V(root_dir_phys);
    for (int i = 0; i < 512; i++) {
        pgd->entries[i] = 0;
    }

    return root_dir_phys;
}

// ARM64 Translation Table Level 0 -> L1 -> L2 -> L3
int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U || paddr == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    // Using 4KB page size, 48-bit address space
    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF; // L0
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF; // L1
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF; // L2
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF; // L3

    pgd_t* pgd = (pgd_t*)P2V(root_table);

    if ((pgd->entries[pgd_idx] & ARM64_MMU_FLAG_VALID) == 0) {
        phys_addr_t new_pud = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pud) return -2;
        pud_t* pud_ptr = (pud_t*)P2V(new_pud);
        for(int i=0; i<512; i++) pud_ptr->entries[i] = 0;
        pgd->entries[pgd_idx] = new_pud | ARM64_MMU_DESCRIPTOR_TABLE;
    }

    pud_t* pud = (pud_t*)P2V(pgd->entries[pgd_idx] & ~0xFFFULL);

    if ((pud->entries[pud_idx] & ARM64_MMU_FLAG_VALID) == 0) {
        phys_addr_t new_pmd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pmd) return -2;
        pmd_t* pmd_ptr = (pmd_t*)P2V(new_pmd);
        for(int i=0; i<512; i++) pmd_ptr->entries[i] = 0;
        pud->entries[pud_idx] = new_pmd | ARM64_MMU_DESCRIPTOR_TABLE;
    }

    pmd_t* pmd = (pmd_t*)P2V(pud->entries[pud_idx] & ~0xFFFULL);

    if ((pmd->entries[pmd_idx] & ARM64_MMU_FLAG_VALID) == 0) {
        phys_addr_t new_pte = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pte) return -2;
        pt_t* pte_ptr = (pt_t*)P2V(new_pte);
        for(int i=0; i<512; i++) pte_ptr->entries[i] = 0;
        pmd->entries[pmd_idx] = new_pte | ARM64_MMU_DESCRIPTOR_TABLE;
    }

    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ~0xFFFULL);

    // Create final Page Descriptor for L3 mapping
    uint64_t mmu_flags = convert_flags_to_arm64(flags);
    pte->entries[pte_idx] = aligned_paddr | ARM64_MMU_DESCRIPTOR_PAGE | mmu_flags;

    return 0;
}

int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr) {
    if (root_table == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF;

    pgd_t* pgd = (pgd_t*)P2V(root_table);
    if ((pgd->entries[pgd_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pud_t* pud = (pud_t*)P2V(pgd->entries[pgd_idx] & ~0xFFFULL);
    if ((pud->entries[pud_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pmd_t* pmd = (pmd_t*)P2V(pud->entries[pud_idx] & ~0xFFFULL);
    if ((pmd->entries[pmd_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ~0xFFFULL);

    if (unmapped_paddr) {
        *unmapped_paddr = pte->entries[pte_idx] & ~0xFFFULL;
    }

    pte->entries[pte_idx] = 0;
    return 0;
}

phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pgd_t* pgd = (pgd_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        pgd->entries[i] = 0;
    }

    if (kernel_root_table != 0U) {
        pgd_t* kernel_pgd = (pgd_t*)P2V(kernel_root_table);
        // ARM64 separates user and kernel spaces via TTBR0 and TTBR1 registers.
        // However, for systems that merge them or use a shared single table space
        // for simplicity (like higher half), we copy the top half mappings.
        // We will copy the highest entry or top half entries to mirror kernel space.
        for(int i = 256; i < 512; i++) {
            pgd->entries[i] = kernel_pgd->entries[i];
        }
    }

    return root;
}

int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    if (root_table == 0U) return -1;

    uint64_t pgd_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (vaddr >> 12) & 0x1FF;

    pgd_t* pgd = (pgd_t*)P2V(root_table);
    if ((pgd->entries[pgd_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pud_t* pud = (pud_t*)P2V(pgd->entries[pgd_idx] & ~0xFFFULL);
    if ((pud->entries[pud_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pmd_t* pmd = (pmd_t*)P2V(pud->entries[pud_idx] & ~0xFFFULL);
    if ((pmd->entries[pmd_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ~0xFFFULL);

    uint64_t old_entry = pte->entries[pte_idx];
    if ((old_entry & ARM64_MMU_FLAG_VALID) == 0) return -2;

    if (paddr) *paddr = old_entry & ~0xFFFULL;
    if (flags) *flags = convert_arm64_to_flags(old_entry);

    return 0;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF;

    pgd_t* pgd = (pgd_t*)P2V(root_table);
    if ((pgd->entries[pgd_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pud_t* pud = (pud_t*)P2V(pgd->entries[pgd_idx] & ~0xFFFULL);
    if ((pud->entries[pud_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pmd_t* pmd = (pmd_t*)P2V(pud->entries[pud_idx] & ~0xFFFULL);
    if ((pmd->entries[pmd_idx] & ARM64_MMU_FLAG_VALID) == 0) return -2;

    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ~0xFFFULL);

    uint64_t mmu_flags = convert_flags_to_arm64(flags);
    pte->entries[pte_idx] = aligned_paddr | ARM64_MMU_DESCRIPTOR_PAGE | mmu_flags;

    return 0;
}
