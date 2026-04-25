#include "../../kernel/include/hal/hal_mpa.h"
#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm.h"
#include "../../kernel/include/numa.h"
#include "../../kernel/include/mm/physmap.h"
#include "../../kernel/include/arch/memops.h"
#include <stdbool.h>

// Direct-Map Subsystem Configuration
// For ARM64, the kernel virtual offset (high-half map)
const virt_addr_t g_kernel_virt_offset = 0xFFFF800000000000ULL;
const size_t g_kernel_physmap_size = 0x8000000000ULL; // e.g., 512GB

typedef enum {
    ARM64_KVA_MODE_EARLY_IDENTITY = 0,
    ARM64_KVA_MODE_LINEAR_MAP = 1,
} arm64_kva_mode_t;

typedef struct {
    arm64_kva_mode_t mode;
    phys_addr_t phys_base;
    virt_addr_t virt_base;
    size_t size;
    bool ready;
} arm64_kernel_map_state_t;

static arm64_kernel_map_state_t g_arm64_kernel_map = {
    .mode = ARM64_KVA_MODE_EARLY_IDENTITY,
    .phys_base = 0,
    .virt_base = 0,
    .size = 0,
    .ready = false,
};

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

#define ARM64_PAGE_MASK      (0x0000FFFFFFFFF000ULL)

// Arch-private raw descriptor
typedef uint64_t pte_raw_t;

typedef struct {
    pte_raw_t entries[512];
} pt_t;

static inline void arm64_pt_zero_table(void *tbl, size_t sz) {
    arch_memset(tbl, 0, sz, ARCH_MEMOP_F_DEFAULT);
}

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

static phys_addr_t arm64_pt_create_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pt_t* pgd = (pt_t*)physmap_phys_to_virt(root);
    arm64_pt_zero_table(pgd, sizeof(*pgd));

    // On arm64, kernel executes at low addresses (0x40000000), covered by entry 0.
    // Entry 0 contains 1GB block mappings that cannot be subdivided.
    // 
    // Strategy:
    // - Kernel space (first creation): Copy entry 0 + entries 256-511
    // - User domains: Copy ONLY entries 256-511, leave entry 0 empty for user mappings
    // 
    // User domains don't execute kernel code at low addresses, so they don't need
    // entry 0. This allows them to create their own fine-grained mappings in the
    // 0-512GB range without conflicting with kernel's 1GB blocks.
    if (kernel_root_table != 0U) {
        extern int vmm_is_kernel_space_ready(void);
        pt_t* kernel_pgd = (pt_t*)physmap_phys_to_virt(kernel_root_table);
        
        // Copy entry 0 ONLY for kernel space creation (before kernel_space_ready)
        if (!vmm_is_kernel_space_ready()) {
            pgd->entries[0] = kernel_pgd->entries[0];
        }
        
        // Always copy kernel half (256-511) for high canonical kernel mappings
        for(int i = 256; i < 512; i++) {
            pgd->entries[i] = kernel_pgd->entries[i];
        }
    } else {
        // Bootstrap the initial kernel root with a 1GB block identity map
        // to cover QEMU's default physical layout.
        // PGD -> PUD
        phys_addr_t pud_pa = mm_alloc_page(NUMA_NODE_ANY);
        if (pud_pa) {
            pt_t* pud = (pt_t*)physmap_phys_to_virt(pud_pa);
            arm64_pt_zero_table(pud, sizeof(*pud));

            // Map 0x00000000 - 0x3FFFFFFF as Device memory (1GB Block)
            uint64_t dev_flags = ARM64_PT_VALID | ARM64_PT_BLOCK | ARM64_PT_AF;
            dev_flags |= ARM64_PT_AP_RW_EL1 | ARM64_PT_PXN | ARM64_PT_UXN;
            dev_flags |= ARM64_PT_ATTR_IDX(MAIR_IDX_DEVICE) | ARM64_PT_OUTER_SHARE;
            pud->entries[0] = 0x00000000ULL | dev_flags;

            // Map 0x40000000 - 0x7FFFFFFF as Normal memory (1GB Block)
            uint64_t ram_flags = ARM64_PT_VALID | ARM64_PT_BLOCK | ARM64_PT_AF;
            ram_flags |= ARM64_PT_AP_RW_EL1; // Executable kernel
            ram_flags |= ARM64_PT_ATTR_IDX(MAIR_IDX_NORMAL) | ARM64_PT_INNER_SHARE;
            pud->entries[1] = 0x40000000ULL | ram_flags;

            // Map 0x80000000 - 0xBFFFFFFF as Normal memory (1GB Block)
            pud->entries[2] = 0x80000000ULL | ram_flags;

            // Map 0xC0000000 - 0xFFFFFFFF as Normal memory (1GB Block)
            pud->entries[3] = 0xC0000000ULL | ram_flags;

            // Link PUD into PGD at index 0 (covers 0x0 to 0x7FFFFFFFFF)
            pgd->entries[0] = pud_pa | ARM64_PT_VALID | ARM64_PT_TABLE;
        }
    }

    return root;
}

static void arm64_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 1) {
        pt_t* pt = (pt_t*)physmap_phys_to_virt(table);
        // Free only user space (entries 0-255 at PGD level, 0-511 at other levels)
        int max_idx = (level == 4) ? 256 : 512;
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

static void arm64_pt_destroy_address_space(phys_addr_t root_pt) {
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

    pt_t* pgd = (pt_t*)physmap_phys_to_virt(root_pt);

    uint64_t table_flags = ARM64_PT_VALID | ARM64_PT_TABLE; // Intermediate tables

    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) {
        phys_addr_t new_pud = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pud) return -2;
        pt_t* pud_ptr = (pt_t*)physmap_phys_to_virt(new_pud);
        arm64_pt_zero_table(pud_ptr, sizeof(*pud_ptr));
        pgd->entries[pgd_idx] = new_pud | table_flags;
    }

    pt_t* pud = (pt_t*)physmap_phys_to_virt(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) {
        phys_addr_t new_pmd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pmd) return -2;
        pt_t* pmd_ptr = (pt_t*)physmap_phys_to_virt(new_pmd);
        arm64_pt_zero_table(pmd_ptr, sizeof(*pmd_ptr));
        pud->entries[pud_idx] = new_pmd | table_flags;
    }

    pt_t* pmd = (pt_t*)physmap_phys_to_virt(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) {
        phys_addr_t new_pte = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pte) return -2;
        pt_t* pte_ptr = (pt_t*)physmap_phys_to_virt(new_pte);
        arm64_pt_zero_table(pte_ptr, sizeof(*pte_ptr));
        pmd->entries[pmd_idx] = new_pte | table_flags;
    }

    pt_t* pte = (pt_t*)physmap_phys_to_virt(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

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

    pt_t* pgd = (pt_t*)physmap_phys_to_virt(root_pt);
    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pud = (pt_t*)physmap_phys_to_virt(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pmd = (pt_t*)physmap_phys_to_virt(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pte = (pt_t*)physmap_phys_to_virt(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

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

    pt_t* pgd = (pt_t*)physmap_phys_to_virt(root_pt);
    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pud = (pt_t*)physmap_phys_to_virt(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pmd = (pt_t*)physmap_phys_to_virt(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pte = (pt_t*)physmap_phys_to_virt(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

    if ((pte->entries[pte_idx] & ARM64_PT_VALID) == 0) return -2;

    uint64_t paddr = pte->entries[pte_idx] & ARM64_PAGE_MASK;
    uint64_t pte_flags = flags_to_arm64(new_flags);

    // Break-before-make remap path
    pte->entries[pte_idx] = 0;
    if (active_hal_tlb) active_hal_tlb->flush_page_local(aligned_vaddr); // Requires local flush

    pte->entries[pte_idx] = paddr | pte_flags;

    return 0;
}

static int arm64_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pgd_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pud_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pmd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pte_idx = (aligned_vaddr >> 12) & 0x1FF;

    pt_t* pgd = (pt_t*)physmap_phys_to_virt(root_pt);
    if ((pgd->entries[pgd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pud = (pt_t*)physmap_phys_to_virt(pgd->entries[pgd_idx] & ARM64_PAGE_MASK);
    if ((pud->entries[pud_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pmd = (pt_t*)physmap_phys_to_virt(pud->entries[pud_idx] & ARM64_PAGE_MASK);
    if ((pmd->entries[pmd_idx] & ARM64_PT_VALID) == 0) return -2;
    pt_t* pte = (pt_t*)physmap_phys_to_virt(pmd->entries[pmd_idx] & ARM64_PAGE_MASK);

    if ((pte->entries[pte_idx] & ARM64_PT_VALID) == 0) return -2;

    if (paddr) *paddr = pte->entries[pte_idx] & ARM64_PAGE_MASK;
    if (flags) *flags = arm64_to_flags(pte->entries[pte_idx] & ~ARM64_PAGE_MASK);

    return 0;
}

static int arm64_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    size_t done = 0;
    while (done < size) {
        int rc = arm64_pt_map_4k(root_pt, vaddr + done, paddr + done, flags);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int arm64_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    size_t done = 0;
    while (done < size) {
        int rc = arm64_pt_unmap_4k(root_pt, vaddr + done, NULL);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int arm64_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    size_t done = 0;
    while (done < size) {
        int rc = arm64_pt_protect_4k(root_pt, vaddr + done, new_flags);
        if (rc != 0) return rc;
        done += PAGE_SIZE;
    }
    return 0;
}

static int arm64_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return arm64_pt_map_range(root_pt, vaddr, paddr, PAGE_SIZE, flags);
}

static int arm64_pt_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (unmapped_paddr) {
        (void)arm64_pt_query_page(root_pt, vaddr, unmapped_paddr, NULL);
    }
    return arm64_pt_unmap_range(root_pt, vaddr, PAGE_SIZE);
}

static int arm64_pt_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    return arm64_pt_protect_range(root_pt, vaddr, PAGE_SIZE, new_flags);
}

static int arm64_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
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

static translate_backend_kind_t arm64_backend_type(void) { return TRANSLATE_BACKEND_MMU; }
static translate_exec_class_t arm64_exec_class(void) { return TRANSLATE_EXEC_MMU_FULL; }

static bool arm64_pa_in_early_identity_window(phys_addr_t pa) {
    return pa < g_kernel_virt_offset;
}

static bool arm64_pa_in_linear_map_range(phys_addr_t pa) {
    if (pa < g_arm64_kernel_map.phys_base) {
        return false;
    }
    size_t delta = (size_t)(pa - g_arm64_kernel_map.phys_base);
    return delta < g_arm64_kernel_map.size;
}

static bool arm64_va_is_early_identity(virt_addr_t va) {
    return va < g_kernel_virt_offset;
}

static bool arm64_va_is_linear_map(virt_addr_t va) {
    if (va < g_arm64_kernel_map.virt_base) {
        return false;
    }
    size_t delta = (size_t)(va - g_arm64_kernel_map.virt_base);
    return delta < g_arm64_kernel_map.size;
}

void arm64_set_kernel_linear_map_state(phys_addr_t phys_base, size_t size, bool enabled) {
    if (!enabled || size == 0) {
        g_arm64_kernel_map.mode = ARM64_KVA_MODE_EARLY_IDENTITY;
        g_arm64_kernel_map.phys_base = 0;
        g_arm64_kernel_map.virt_base = 0;
        g_arm64_kernel_map.size = 0;
        g_arm64_kernel_map.ready = false;
        return;
    }

    g_arm64_kernel_map.mode = ARM64_KVA_MODE_LINEAR_MAP;
    g_arm64_kernel_map.phys_base = phys_base;
    g_arm64_kernel_map.virt_base = g_kernel_virt_offset + phys_base;
    g_arm64_kernel_map.size = size;
    g_arm64_kernel_map.ready = true;
}

static void* arm64_phys_to_virt(phys_addr_t phys) {
    switch (g_arm64_kernel_map.mode) {
    case ARM64_KVA_MODE_EARLY_IDENTITY:
        if (!arm64_pa_in_early_identity_window(phys)) {
            return NULL;
        }
        return (void*)(uintptr_t)phys;

    case ARM64_KVA_MODE_LINEAR_MAP:
        if (!g_arm64_kernel_map.ready) {
            return NULL;
        }
        if (!arm64_pa_in_linear_map_range(phys)) {
            return NULL;
        }
        size_t delta = (size_t)(phys - g_arm64_kernel_map.phys_base);
        return (void*)(uintptr_t)(g_arm64_kernel_map.virt_base + delta);

    default:
        return NULL;
    }
}

static phys_addr_t arm64_virt_to_phys(const void* virt) {
    uintptr_t va = (uintptr_t)virt;

    switch (g_arm64_kernel_map.mode) {
    case ARM64_KVA_MODE_EARLY_IDENTITY:
        if (!arm64_va_is_early_identity((virt_addr_t)va)) {
            return 0;
        }
        return (phys_addr_t)va;

    case ARM64_KVA_MODE_LINEAR_MAP:
        if (!g_arm64_kernel_map.ready) {
            return 0;
        }
        if (!arm64_va_is_linear_map((virt_addr_t)va)) {
            return 0;
        }
        size_t delta = (size_t)(va - g_arm64_kernel_map.virt_base);
        return g_arm64_kernel_map.phys_base + delta;

    default:
        return 0;
    }
}

static bool arm64_has_linear_physmap(void) { return true; }
static phys_addr_t arm64_linear_physmap_base(void) {
    if (g_arm64_kernel_map.mode == ARM64_KVA_MODE_LINEAR_MAP) {
        return g_arm64_kernel_map.virt_base;
    }
    return 0;
}
static phys_addr_t arm64_linear_physmap_limit(void) {
    if (g_arm64_kernel_map.mode == ARM64_KVA_MODE_LINEAR_MAP) {
        return g_arm64_kernel_map.virt_base + g_arm64_kernel_map.size;
    }
    return 0;
}

static const hal_translate_ops_t arm64_translate_ops = {
    .backend_type = arm64_backend_type,
    .exec_class = arm64_exec_class,
    .phys_to_virt = arm64_phys_to_virt,
    .virt_to_phys = arm64_virt_to_phys,
    .has_linear_physmap = arm64_has_linear_physmap,
    .linear_physmap_base = arm64_linear_physmap_base,
    .linear_physmap_limit = arm64_linear_physmap_limit,
};

const hal_translate_ops_t* hal_translate_ops(void) {
    return &arm64_translate_ops;
}

static const hal_pt_caps_t arm64_pt_caps = {
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
    .supports_nx_or_xn = true,
    .supports_ad_bits = true,
    .supports_large_2m = true,
    .supports_large_1g = false,
    .supports_device_memtype = true,
    .supports_writecombine = false,
    .requires_bbm = true,
    .supports_cow_softbit = true,
    .supports_linear_physmap = true,
};

extern hal_tlb_ops_t arm64_hal_tlb_ops;

hal_pt_ops_t arm64_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MMU,
    .caps                  = &arm64_pt_caps,
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

void arch_hal_pt_init(void) {
    hal_pt_register_ops(&arm64_hal_pt_ops, &arm64_hal_tlb_ops);
}

static const hal_tlb_caps_t arm64_tlb_caps = {
    .supports_page_flush = true,
    .supports_range_flush = true,
    .supports_aspace_flush = true,
    .supports_all_flush = true,
    .supports_remote_targeted_flush = false,
    .supports_broadcast_flush = true,
    .supports_asid_selective_flush = true,
    .supports_lazy_generation_model = false,
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
    .caps                 = &arm64_tlb_caps,
    .flush_page_local      = arm64_tlb_flush_page_local,
    .flush_all_local       = arm64_tlb_flush_all_local,
    .flush_asid_local      = arm64_tlb_flush_asid_local,
    .flush_page_remote     = arm64_tlb_flush_page_remote,
    .flush_all_remote      = arm64_tlb_flush_all_remote,
    .flush_page_broadcast  = arm64_tlb_flush_page_broadcast,
    .flush_all_broadcast   = arm64_tlb_flush_all_broadcast,
};

// Implement the specific functions from mem_protect_cpu_ops

static phys_addr_t arm64_mpa_make_table(uint32_t level) {
    (void)level;
    // Just map to the page allocator like the original pt did
    phys_addr_t pt = mm_alloc_page(NUMA_NODE_ANY);
    if (pt) {
        pt_t* ptr = (pt_t*)physmap_phys_to_virt(pt);
        for(int i = 0; i < 512; i++) ptr->entries[i] = 0;
    }
    return pt;
}

static int arm64_mpa_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    uint32_t hal_flags = HAL_PT_FLAG_READ;
    if (flags & MPA_CAP_EXEC_PERM) hal_flags |= HAL_PT_FLAG_EXEC;
    if (flags & MPA_CAP_USER) hal_flags |= HAL_PT_FLAG_USER;
    if (flags & MPA_CAP_GLOBAL) hal_flags |= HAL_PT_FLAG_GLOBAL;
    if (flags & MPA_CAP_DEVICE) hal_flags |= HAL_PT_FLAG_DEVICE;
    if (flags & MPA_CAP_WRITE) hal_flags |= HAL_PT_FLAG_WRITE;

    return arm64_pt_map_4k(root_pt, vaddr, paddr, hal_flags);
}

static int arm64_mpa_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    return arm64_pt_unmap_4k(root_pt, vaddr, unmapped_paddr);
}

static void arm64_mpa_set_root(phys_addr_t root) {
    // TCR_EL1: T0SZ=16, T1SZ=16 (48-bit VA), TG0=0 (4KB), TG1=2 (4KB), IPS=2 (40-bit PA)
    uint64_t tcr = (16ULL << 0) | (16ULL << 16) | (3ULL << 12) | (3ULL << 28) |
                   (1ULL << 10) | (1ULL << 26) | (1ULL << 8) | (1ULL << 24) |
                   (0ULL << 14) | (2ULL << 30) | (2ULL << 32);
    
    // MAIR_EL1: Attr0=Normal, Attr1=Device-nGnRE, Attr2=Device-nGnRnE
    uint64_t mair = (0xFFLL << 0) | (0x04LL << 8) | (0x00LL << 16);

    asm volatile(
        "msr mair_el1, %1\n"
        "msr tcr_el1, %2\n"
        "msr ttbr0_el1, %0\n"
        "msr ttbr1_el1, %0\n"
        "isb\n"
        "mrs x0, sctlr_el1\n"
        "bic x0, x0, #2\n"   /* Clear A (alignment check trap) for kernel EL1 */
        "orr x0, x0, #1\n"
        "msr sctlr_el1, x0\n"
        "isb\n"
        :: "r"((uintptr_t)root), "r"(mair), "r"(tcr)
        : "x0", "memory"
    );
}

static void arm64_mpa_flush_tlb_local(virt_addr_t vaddr, uint16_t asid) {
    (void)asid; // Minimal implementation for now
    __asm__ volatile(
        "tlbi vale1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(vaddr >> 12)
    );
}

static phys_addr_t arm64_mpa_get_root(void) {
    uint64_t sctlr;
    phys_addr_t ttbr1;

    /*
     * When MMU is disabled (SCTLR_EL1.M == 0), TTBR values are not an
     * authoritative active translation root for VMM bootstrap.
     */
    __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    if ((sctlr & 1ULL) == 0ULL) {
        return 0U;
    }

    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1));
    return ttbr1 & ~(0xFFFULL);
}

mem_protect_ops_t arm64_mem_protect_ops = {
    .supported_caps = MPA_CAP_VIRT | MPA_CAP_ASID | MPA_CAP_GLOBAL | MPA_CAP_HUGEPAGE | MPA_CAP_EXEC_PERM | MPA_CAP_WRITE | MPA_CAP_USER | MPA_CAP_DEVICE,
    .cpu_ops = {
        .make_table = arm64_mpa_make_table,
        .map_page = arm64_mpa_map_page,
        .unmap_page = arm64_mpa_unmap_page,
        .set_root = arm64_mpa_set_root,
        .flush_tlb_local = arm64_mpa_flush_tlb_local,
        .get_root = arm64_mpa_get_root,
    },
    .iommu_ops = {
        .probe = NULL, // Could be linked to SMMU probe later
    }
};

mem_protect_ops_t *active_mem_protect = &arm64_mem_protect_ops;
