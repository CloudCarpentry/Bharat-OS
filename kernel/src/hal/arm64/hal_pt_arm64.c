#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm.h"
#include "../../../include/numa.h"
#include <stdbool.h>

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

// ARM64 Descriptor bits
#define ARM64_PT_VALID       (1ULL << 0)
#define ARM64_PT_TABLE       (1ULL << 1)
#define ARM64_PT_PAGE        (1ULL << 1) // level 3
#define ARM64_PT_BLOCK       (0ULL << 1) // level 0,1,2

// Access permissions
#define ARM64_PT_AP_RW_EL1   (0ULL << 6)
#define ARM64_PT_AP_RW_EL0   (1ULL << 6)
#define ARM64_PT_AP_RO_EL1   (2ULL << 6)
#define ARM64_PT_AP_RO_EL0   (3ULL << 6)

// Memory Attributes (MAIR Index)
#define ARM64_PT_ATTR_IDX(x) (((uint64_t)(x) & 0x7) << 2)
#define MAIR_IDX_NORMAL      0
#define MAIR_IDX_DEVICE      1
#define MAIR_IDX_NOCACHE     2

// Shareability
#define ARM64_PT_NON_SHARE   (0ULL << 8)
#define ARM64_PT_OUTER_SHARE (2ULL << 8)
#define ARM64_PT_INNER_SHARE (3ULL << 8)

// Execution
#define ARM64_PT_UXN         (1ULL << 54)
#define ARM64_PT_PXN         (1ULL << 53)

// Access flag
#define ARM64_PT_AF          (1ULL << 10)

#define ARM64_PAGE_MASK      (~0xFFFULL)

typedef struct {
    uint64_t entries[512];
} pt_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & ARM64_PAGE_MASK;
}

static bool table_empty(pt_t* table) {
    for (size_t i = 0; i < 512; i++) {
        if (table->entries[i] & ARM64_PT_VALID) {
            return false;
        }
    }
    return true;
}

static uint64_t flags_to_arm64(uint32_t flags) {
    uint64_t pte_flags = ARM64_PT_VALID | ARM64_PT_PAGE | ARM64_PT_AF | ARM64_PT_INNER_SHARE;

    // Permissions
    if (flags & HAL_PT_FLAG_USER) {
        if (flags & HAL_PT_FLAG_WRITE) {
            pte_flags |= ARM64_PT_AP_RW_EL0;
        } else {
            pte_flags |= ARM64_PT_AP_RO_EL0;
        }
        if (!(flags & HAL_PT_FLAG_EXEC)) {
            pte_flags |= ARM64_PT_UXN; // UXN if not executable
        }
        // PXN is generally set for user pages to prevent kernel from executing them (Privileged eXecute Never)
        pte_flags |= ARM64_PT_PXN;
    } else {
        if (flags & HAL_PT_FLAG_WRITE) {
            pte_flags |= ARM64_PT_AP_RW_EL1;
        } else {
            pte_flags |= ARM64_PT_AP_RO_EL1;
        }
        if (!(flags & HAL_PT_FLAG_EXEC)) {
            pte_flags |= ARM64_PT_PXN;
        }
        pte_flags |= ARM64_PT_UXN; // UXN set for kernel pages
    }

    // Memory Attributes (MAIR Index)
    if (flags & HAL_PT_FLAG_DEVICE) {
        pte_flags |= ARM64_PT_ATTR_IDX(MAIR_IDX_DEVICE);
        pte_flags &= ~ARM64_PT_INNER_SHARE;
        pte_flags |= ARM64_PT_OUTER_SHARE; // Device memory is usually outer shareable
    } else if (flags & HAL_PT_FLAG_NOCACHE) {
        pte_flags |= ARM64_PT_ATTR_IDX(MAIR_IDX_NOCACHE);
    } else {
        pte_flags |= ARM64_PT_ATTR_IDX(MAIR_IDX_NORMAL);
    }

    return pte_flags;
}

static uint32_t arm64_to_flags(uint64_t pte_flags) {
    uint32_t flags = HAL_PT_FLAG_READ;

    uint64_t ap = (pte_flags >> 6) & 0x3;
    if (ap == 1 || ap == 3) {
        flags |= HAL_PT_FLAG_USER;
    }

    if (ap == 0 || ap == 1) {
        flags |= HAL_PT_FLAG_WRITE;
    }

    if (flags & HAL_PT_FLAG_USER) {
        if (!(pte_flags & ARM64_PT_UXN)) flags |= HAL_PT_FLAG_EXEC;
    } else {
        if (!(pte_flags & ARM64_PT_PXN)) flags |= HAL_PT_FLAG_EXEC;
    }

    uint64_t attr_idx = (pte_flags >> 2) & 0x7;
    if (attr_idx == MAIR_IDX_DEVICE) {
        flags |= HAL_PT_FLAG_DEVICE;
    } else if (attr_idx == MAIR_IDX_NOCACHE) {
        flags |= HAL_PT_FLAG_NOCACHE;
    }

    return flags;
}

phys_addr_t arm64_pt_create_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pt_t* pgd = (pt_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        pgd->entries[i] = 0;
    }

    // In ARM64, we usually separate User and Kernel address spaces via TTBR0 and TTBR1.
    // If a shared address space is used, we'd copy the kernel upper half.
    if (kernel_root_table != 0U) {
        pt_t* kernel_pgd = (pt_t*)P2V(kernel_root_table);
        for(int i = 256; i < 512; i++) {
            pgd->entries[i] = kernel_pgd->entries[i];
        }
    }

    return root;
}

static void arm64_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 1) {
        pt_t* pt = (pt_t*)P2V(table);
        int max_idx = (level == 4) ? 256 : 512; // Free only bottom half (User) of PGD
        for (int i = 0; i < max_idx; i++) {
            if (pt->entries[i] & ARM64_PT_VALID) {
                // If it's a block descriptor at level 1 or 2, don't recurse
                if ((level == 3 || level == 2) && ((pt->entries[i] & 2) == 0)) {
                    continue;
                }
                arm64_pt_destroy_recursive(pt->entries[i] & ARM64_PAGE_MASK, level - 1);
            }
        }
    }
    mm_free_page(table);
}

void arm64_pt_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        arm64_pt_destroy_recursive(root_pt, 4);
    }
}

static int arm64_pt_map_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pgd = (pt_t*)P2V(root_pt);

    uint64_t table_flags = ARM64_PT_VALID | ARM64_PT_TABLE; // Intermediate tables

    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) {
        phys_addr_t new_pud = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pud) return -2;
        pt_t* pud_ptr = (pt_t*)P2V(new_pud);
        for(int i=0; i<512; i++) pud_ptr->entries[i] = 0;
        pgd->entries[pgd_idx] = new_pud | table_flags;
    }

    pt_t* pud = (pt_t*)P2V(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) {
        phys_addr_t new_pmd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pmd) return -2;
        pt_t* pmd_ptr = (pt_t*)P2V(new_pmd);
        for(int i=0; i<512; i++) pmd_ptr->entries[i] = 0;
        pud->entries[pud_idx] = new_pmd | table_flags;
    }

    pt_t* pmd = (pt_t*)P2V(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) {
        phys_addr_t new_pte = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pte) return -2;
        pt_t* pte_ptr = (pt_t*)P2V(new_pte);
        for(int i=0; i<512; i++) pte_ptr->entries[i] = 0;
        pmd->entries[pmd_idx] = new_pte | table_flags;
    }

    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

    uint64_t pte_flags = flags_to_arm64(flags);
    pte->entries[pte_idx] = aligned_paddr | pte_flags;

    return 0;
}

static int arm64_pt_unmap_4k(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pgd = (pt_t*)P2V(root_pt);
    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pud = (pt_t*)P2V(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pmd = (pt_t*)P2V(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

    if ((pte->entries[pte_idx] & ARM64_PT_VALID) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = pte->entries[pte_idx] & ARM64_PAGE_MASK;
    }

    // Break-before-make remap path requires clearing the entry, then invalidating TLB
    pte->entries[pte_idx] = 0;

    if (table_empty(pte)) {
        mm_free_page(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);
        pmd->entries[pmd_idx] = 0;
        if (table_empty(pmd)) {
            mm_free_page(pud->entries[pud_idx] & ARM64_PAGE_MASK);
            pud->entries[pud_idx] = 0;
            if (table_empty(pud)) {
                mm_free_page(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
                pgd->entries[pgd_idx] = 0;
            }
        }
    }

    return 0;
}

static int arm64_pt_protect_4k(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pgd = (pt_t*)P2V(root_pt);
    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pud = (pt_t*)P2V(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pmd = (pt_t*)P2V(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

    if ((pte->entries[pte_idx] & ARM64_PT_VALID) == 0) return -2;

    uint64_t paddr = pte->entries[pte_idx] & ARM64_PAGE_MASK;
    uint64_t pte_flags = flags_to_arm64(new_flags);

    // Break-before-make remap path
    pte->entries[pte_idx] = 0;
    if (active_hal_tlb) active_hal_tlb->flush_page_local(aligned_vaddr); // Requires local flush

    pte->entries[pte_idx] = paddr | pte_flags;

    return 0;
}

int arm64_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pgd = (pt_t*)P2V(root_pt);
    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pud = (pt_t*)P2V(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pmd = (pt_t*)P2V(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pte = (pt_t*)P2V(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

    if ((pte->entries[pte_idx] & ARM64_PT_VALID) == 0) return -2;

    if (paddr) *paddr = pte->entries[pte_idx] & ARM64_PAGE_MASK;
    if (flags) *flags = arm64_to_flags(pte->entries[pte_idx] & ~ARM64_PAGE_MASK);

    return 0;
}

int arm64_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    size_t done = 0;
    while (done < size) {
        int rc = arm64_pt_map_4k(root_pt, vaddr + done, paddr + done, flags);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

int arm64_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    size_t done = 0;
    while (done < size) {
        int rc = arm64_pt_unmap_4k(root_pt, vaddr + done, NULL);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

int arm64_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    size_t done = 0;
    while (done < size) {
        int rc = arm64_pt_protect_4k(root_pt, vaddr + done, new_flags);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

int arm64_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return arm64_pt_map_range(root_pt, vaddr, paddr, PAGE_SIZE, flags);
}

int arm64_pt_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (unmapped_paddr) {
        (void)arm64_pt_query_page(root_pt, vaddr, unmapped_paddr, NULL);
    }
    return arm64_pt_unmap_range(root_pt, vaddr, PAGE_SIZE);
}

int arm64_pt_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    return arm64_pt_protect_range(root_pt, vaddr, PAGE_SIZE, new_flags);
}

int arm64_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
    int rc = arm64_pt_query_page(root_pt, vaddr, paddr, flags);
    if (rc != 0) return rc;
    if (mapped_size) *mapped_size = PAGE_SIZE;
    return 0;
}

void arm64_init_hardening(void) {
    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    // PAN (Privileged Access Never) is typically bit 23 in SCTLR_EL1 (if ARMv8.1)
    // For older cores, this might do nothing or trigger undef if PAN is not supported,
    // so we need to read ID_AA64MMFR1_EL1 to check if PAN is supported.
    uint64_t mmfr1;
    asm volatile("mrs %0, id_aa64mmfr1_el1" : "=r"(mmfr1));
    if ((mmfr1 >> 20) & 0xF) { // PAN supported
        // asm volatile("msr pan, #1"); // Turn on PAN
    }
}

hal_pt_ops_t arm64_hal_pt_ops = {
    .create_address_space  = arm64_pt_create_address_space,
    .destroy_address_space = arm64_pt_destroy_address_space,
    .map_page              = arm64_pt_map_page,
    .unmap_page            = arm64_pt_unmap_page,
    .protect_page          = arm64_pt_protect_page,
    .query_page            = arm64_pt_query_page,
    .map_range             = arm64_pt_map_range,
    .unmap_range           = arm64_pt_unmap_range,
    .protect_range         = arm64_pt_protect_range,
    .query_mapping         = arm64_pt_query_mapping,
};

static void arm64_tlb_flush_page_local(virt_addr_t vaddr) {
    asm volatile(
        "tlbi vale1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(vaddr >> 12)
    );
}

static void arm64_tlb_flush_all_local(void) {
    asm volatile(
        "tlbi vmalle1is\n"
        "dsb ish\n"
        "isb\n"
    );
}

static void arm64_tlb_flush_asid_local(uint16_t asid) {
    asm volatile(
        "tlbi aside1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"((uint64_t)asid << 48)
    );
}

static void arm64_tlb_flush_page_remote(uint16_t target_core, uint16_t asid, virt_addr_t vaddr) {
    (void)target_core;
    (void)asid;
    (void)vaddr;
    // Handled by URPC or architectural broadcast in ARM64 (usually inner shareable domain broadcast)
}

static void arm64_tlb_flush_all_remote(uint16_t target_core, uint16_t asid) {
    (void)target_core;
    (void)asid;
}

static void arm64_tlb_flush_page_broadcast(uint16_t asid, virt_addr_t vaddr) {
    // ASID and VA match
    uint64_t val = ((uint64_t)asid << 48) | (vaddr >> 12);
    asm volatile(
        "tlbi vae1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(val)
    );
}

static void arm64_tlb_flush_all_broadcast(uint16_t asid) {
    asm volatile(
        "tlbi aside1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"((uint64_t)asid << 48)
    );
}

hal_tlb_ops_t arm64_hal_tlb_ops = {
    .flush_page_local      = arm64_tlb_flush_page_local,
    .flush_all_local       = arm64_tlb_flush_all_local,
    .flush_asid_local      = arm64_tlb_flush_asid_local,
    .flush_page_remote     = arm64_tlb_flush_page_remote,
    .flush_all_remote      = arm64_tlb_flush_all_remote,
    .flush_page_broadcast  = arm64_tlb_flush_page_broadcast,
    .flush_all_broadcast   = arm64_tlb_flush_all_broadcast,
};
