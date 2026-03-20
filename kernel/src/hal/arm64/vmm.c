#include "../../../include/hal/vmm.h"
#include "../../../include/hal/mmu_ops.h"
#include "../../../include/numa.h"

// Define direct map macros for early initialization.
// Mimicking the logic used in generic VMM for identity map base.
#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

#define ARM64_MMU_DEVICE_nGnRnE (0ULL << 2) // MAIR index 0 -> Device-nGnRnE
#define ARM64_MMU_NOCACHE_MEM   (1ULL << 2) // MAIR index 1 -> Non-cacheable

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

virt_addr_t align_down(virt_addr_t value) {
    return value & ~(virt_addr_t)(PAGE_SIZE - 1U);
}

// Convert architecture independent flags to ARM64 specific flags
uint64_t convert_flags_to_arm64(uint32_t flags) {
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

uint32_t convert_arm64_to_flags(uint64_t mmu_flags) {
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

#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"

phys_addr_t hal_vmm_init_root(void) {
    if (!active_hal_pt) hal_pt_init();
    return active_hal_pt->create_address_space(0);
}

static uint64_t hal_get_ttbr0(void) {
    uint64_t ttbr0;
    __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0));
    return ttbr0 & ~0xFFFULL;
}

static uint64_t hal_get_ttbr1(void) {
    uint64_t ttbr1;
    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1));
    return ttbr1 & ~0xFFFULL;
}

void hal_tlbi_page(virt_addr_t vaddr) {
    __asm__ volatile(
        "tlbi vale1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(vaddr >> 12)
    );
}

// ARM64 Translation Table Level 0 -> L1 -> L2 -> L3
int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!active_hal_pt) hal_pt_init();

    uint32_t pt_flags = HAL_PT_FLAG_READ;
    if (flags & CAP_RIGHT_WRITE) pt_flags |= HAL_PT_FLAG_WRITE;
    if (flags & PAGE_USER)       pt_flags |= HAL_PT_FLAG_USER;
    if (flags & (CAP_RIGHT_DEVICE_GPU | CAP_RIGHT_DEVICE_NPU)) pt_flags |= HAL_PT_FLAG_DEVICE;




    pt_flags |= HAL_PT_FLAG_EXEC;

    return active_hal_pt->map_page(root_table, vaddr, paddr, pt_flags);
}

// -----------------------------------------------------------------------------
// mmu_ops_t Wrapper implementations
// -----------------------------------------------------------------------------

static phys_addr_t arm64_mmu_create_table(void) {
    return hal_vmm_init_root();
}

static void arm64_mmu_destroy_table_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 1) {
        pt_t* pt = (pt_t*)P2V(table);
        for (int i = 0; i < (level == 4 ? 256 : 512); i++) { // Skip kernel half on pgd
            if (pt->entries[i] & ARM64_MMU_FLAG_VALID) {
                if ((level == 3 || level == 2) && (pt->entries[i] & ARM64_MMU_DESCRIPTOR_TABLE) == ARM64_MMU_DESCRIPTOR_BLOCK) {
                    continue; // Huge page block, don't recurse
                }
                arm64_mmu_destroy_table_recursive(pt->entries[i] & ~0xFFFULL, level - 1);
            }
        }
    }
    mm_free_page(table);
}

static void arm64_mmu_destroy_table(phys_addr_t root) {
    if (root) {
        arm64_mmu_destroy_table_recursive(root, 4);
    }
}

static phys_addr_t arm64_mmu_clone_kernel(phys_addr_t kernel_root) {
    return hal_vmm_setup_address_space(kernel_root);
}

#define ARM64_MMU_DEVICE_nGnRnE (0ULL << 2) // MAIR index 0 -> Device-nGnRnE
#define ARM64_MMU_NOCACHE_MEM   (1ULL << 2) // MAIR index 1 -> Non-cacheable

// Translate generic mmu_flags_t to ARM64-specific VMM flags
static uint32_t flags_to_arm64(mmu_flags_t f) {
    uint32_t flags = 0;
    if (f & MMU_WRITE)   flags |= CAP_RIGHT_WRITE;
    if (f & MMU_USER)    flags |= PAGE_USER;
    if (f & MMU_COW)     flags |= PAGE_COW;
    if (f & MMU_DEVICE)  flags |= ARM64_MMU_DEVICE_nGnRnE; // Will be handled in convert_flags_to_arm64
    if (f & MMU_NOCACHE) flags |= ARM64_MMU_NOCACHE_MEM;
    return flags;
}

// Translate ARM64-specific VMM flags to generic mmu_flags_t
static mmu_flags_t arm64_to_flags(uint32_t flags) {
    mmu_flags_t f = MMU_READ;
    if (flags & CAP_RIGHT_WRITE) f |= MMU_WRITE;
    if (flags & PAGE_USER)       f |= MMU_USER;
    if (flags & PAGE_COW)        f |= MMU_COW;
    return f;
}

static int arm64_mmu_map(phys_addr_t root, virt_addr_t virt, phys_addr_t phys,
                         size_t size, mmu_flags_t flags) {
    size_t mapped = 0;
    while (mapped < size) {
        int ret = hal_vmm_map_page(root, virt + mapped, phys + mapped, flags_to_arm64(flags));
        if (ret < 0) return ret;
        mapped += PAGE_SIZE;
    }
    return 0;
}

static int arm64_mmu_unmap(phys_addr_t root, virt_addr_t virt, size_t size, phys_addr_t *unmapped_phys) {
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

static int arm64_mmu_protect(phys_addr_t root, virt_addr_t virt, size_t size, mmu_flags_t new_flags) {
    size_t updated = 0;
    while (updated < size) {
        phys_addr_t phys = 0;
        uint32_t old_flags = 0;
        int ret = hal_vmm_get_mapping(root, virt + updated, &phys, &old_flags);
        if (ret == 0 && phys != 0) {
            hal_vmm_update_mapping(root, virt + updated, phys, flags_to_arm64(new_flags));
        }
        updated += PAGE_SIZE;
    }
    return 0;
}

static int arm64_mmu_query(phys_addr_t root, virt_addr_t virt, phys_addr_t *phys_out, mmu_flags_t *flags_out) {
    uint32_t arm64_flags = 0;
    int ret = hal_vmm_get_mapping(root, virt, phys_out, &arm64_flags);
    if (ret == 0 && flags_out) {
        *flags_out = arm64_to_flags(arm64_flags);
    }
    return ret;
}

static void arm64_mmu_activate(phys_addr_t root) {
    // In Bharat-v3-64, we activate the provided root table in TTBR0.
    // We assume the kernel is identity-mapped (or correctly mapped in the provided table).
    // This is called AFTER we've mapped the RAM and (optionally) the framebuffer.
    
    uint64_t mair = (0xFFLL << 0)  | // Attr 0: Normal WB/WA/RA
                    (0x04LL << 8)  | // Attr 1: Device-nGnRE
                    (0x00LL << 16);  // Attr 2: Device-nGnRnE
    
    // TCR_EL1:
    // T0SZ=16 (48-bit VA)
    // TG0=0 (4KB)
    // SH0=3 (Inner Shareable)
    // ORGN0=1 (Normal WB/WA)
    // IRGN0=1 (Normal WB/WA)
    // IPS=2 (40-bit PA)
    uint64_t tcr = (16ULL << 0) | (3ULL << 12) | (1ULL << 10) | (1ULL << 8) | (0ULL << 14) | (2ULL << 32);

    asm volatile(
        "msr mair_el1, %1\n"
        "msr tcr_el1, %2\n"
        "msr ttbr0_el1, %0\n"
        "isb\n"
        "tlbi vmalle1is\n"
        "dsb ish\n"
        "isb\n"
        "mrs x0, sctlr_el1\n"
        "orr x0, x0, #1\n"      // MMU enable
        "and x0, x0, #~2\n"     // Alignment check (disable SCTLR_EL1.A)
        "msr sctlr_el1, x0\n"
        "isb\n"
        :: "r"((uintptr_t)root), "r"(mair), "r"(tcr)
        : "x0", "memory"
    );
}

static void arm64_mmu_deactivate(void) {
    // Usually means clearing TTBR0_EL1
}

static void arm64_mmu_tlb_flush_page(virt_addr_t virt) {
    asm volatile(
        "tlbi vale1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(virt >> 12)
    );
}

static void arm64_mmu_tlb_flush_all(void) {
    asm volatile(
        "tlbi vmalle1is\n"
        "dsb ish\n"
        "isb\n"
    );
}

static void arm64_mmu_tlb_flush_asid(uint16_t asid) {
    asm volatile(
        "tlbi aside1is, %0\n"
        "dsb ish\n"
        :: "r"((uint64_t)asid << 48)
    );
}

static size_t arm64_huge_pages[] = {0x200000, 0x40000000, 0}; // 2MB, 1GB

mmu_ops_t arm64_mmu_ops = {
    .create_table     = arm64_mmu_create_table,
    .destroy_table    = arm64_mmu_destroy_table,
    .clone_kernel     = arm64_mmu_clone_kernel,
    .map              = arm64_mmu_map,
    .unmap            = arm64_mmu_unmap,
    .protect          = arm64_mmu_protect,
    .query            = arm64_mmu_query,
    .activate         = arm64_mmu_activate,
    .deactivate       = arm64_mmu_deactivate,
    .tlb_flush_page   = arm64_mmu_tlb_flush_page,
    .tlb_flush_all    = arm64_mmu_tlb_flush_all,
    .tlb_flush_asid   = arm64_mmu_tlb_flush_asid,

    .page_size        = 4096,
    .huge_page_sizes  = arm64_huge_pages,
    .levels           = 4, // Assuming 4KB granule, 48-bit PA
    .has_nx           = true,
    .asid_bits        = 16, // Typical for ARMv8
    .has_user_kernel_split = true, // TTBR0 vs TTBR1
};

void arm64_mmu_detect(mmu_ops_t *ops) {
    // Stub for runtime detection (e.g. read id_aa64mmfr0_el1)
    (void)ops;

    extern void arm64_init_hardening(void);
    arm64_init_hardening();
}

int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr) {
    if (!active_hal_pt) hal_pt_init();

    int ret = active_hal_pt->unmap_page(root_table, vaddr, unmapped_paddr);

    if (ret == 0 && (root_table == hal_get_ttbr0() || root_table == hal_get_ttbr1())) {
        if (active_hal_tlb) active_hal_tlb->flush_page_local(vaddr);
    }

    return ret;
}

phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table) {
    if (!active_hal_pt) hal_pt_init();
    return active_hal_pt->create_address_space(kernel_root_table);
}

int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    if (!active_hal_pt) hal_pt_init();

    uint32_t pt_flags = 0;
    int ret = active_hal_pt->query_page(root_table, vaddr, paddr, &pt_flags);

    if (ret == 0 && flags) {
        *flags = ARM64_MMU_FLAG_VALID;
        if (pt_flags & HAL_PT_FLAG_WRITE) *flags |= CAP_RIGHT_WRITE;
        if (pt_flags & HAL_PT_FLAG_USER)  *flags |= PAGE_USER;
    }

    return ret;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!active_hal_pt) hal_pt_init();

    uint32_t pt_flags = HAL_PT_FLAG_READ;
    if (flags & CAP_RIGHT_WRITE) pt_flags |= HAL_PT_FLAG_WRITE;
    if (flags & PAGE_USER)       pt_flags |= HAL_PT_FLAG_USER;
    if (flags & (CAP_RIGHT_DEVICE_GPU | CAP_RIGHT_DEVICE_NPU)) pt_flags |= HAL_PT_FLAG_DEVICE;




    pt_flags |= HAL_PT_FLAG_EXEC;

    int ret = active_hal_pt->map_page(root_table, vaddr, paddr, pt_flags);

    if (ret == 0 && (root_table == hal_get_ttbr0() || root_table == hal_get_ttbr1())) {
        if (active_hal_tlb) active_hal_tlb->flush_page_local(vaddr);
    }

    return ret;
}
