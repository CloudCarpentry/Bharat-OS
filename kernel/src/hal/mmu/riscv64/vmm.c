#include "../../../../include/hal/vmm.h"
#include "../../../../include/hal/mmu_ops.h"
#include "../../../../include/numa.h"
#include "../../../../include/mm/physmap.h"
#include "../../../../include/mm/prot_domain.h"
#include "../../../../include/arch/arch_caps.h"

#define ERR_NOT_SUPPORTED -1

// RISC-V Sv39 Page Table Entry Flags
#define RISCV_PTE_V (1ULL << 0) // Valid
#define RISCV_PTE_R (1ULL << 1) // Read
#define RISCV_PTE_W (1ULL << 2) // Write
#define RISCV_PTE_X (1ULL << 3) // Execute
#define RISCV_PTE_U (1ULL << 4) // User

typedef struct {
    uint64_t entries[512];
} pte_table_t;



#include "../../../../include/hal/hal_pt.h"
#include "../../../../include/hal/hal_tlb.h"

phys_addr_t hal_vmm_init_root(void) {
    if (!active_hal_pt) hal_pt_init();
    return active_hal_pt->create_address_space(0);
}

static uint64_t hal_get_satp(void) {
    uint64_t satp;
    __asm__ volatile("csrr %0, satp" : "=r"(satp));
    // Extracts physical address from satp (assuming Sv39, ppn is lower 44 bits)
    return (satp & ((1ULL << 44) - 1)) << 12;
}

int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!active_hal_pt) hal_pt_init();

    uint32_t pt_flags = HAL_PT_FLAG_READ;
    if (flags & CAP_RIGHT_WRITE) pt_flags |= HAL_PT_FLAG_WRITE;
    if (flags & PAGE_USER)       pt_flags |= HAL_PT_FLAG_USER;

    if (flags & (1U << 30)) {
        pt_flags |= HAL_PT_FLAG_DEVICE;
    }
    if (flags & (1U << 31)) {
        pt_flags |= HAL_PT_FLAG_NOCACHE;
    }

    pt_flags |= HAL_PT_FLAG_EXEC; // Assume executable

    int ret = active_hal_pt->map_page(root_table, vaddr, paddr, pt_flags);
    return ret;
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
        pte_table_t* pt = (pte_table_t*)physmap_phys_to_virt(table);
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

// Ensure BHARAT_ASSERT is available or use equivalent panic
extern void kernel_panic(const char *message);
extern uint64_t g_riscv_satp_mode; // Phase 1 output

static void riscv_mmu_activate(phys_addr_t root) {
    // Mode 8 is Sv39
    if (g_riscv_satp_mode != 8) {
        kernel_panic("hal_vmm_init_root called without confirmed Sv39 support");
    }

    uintptr_t ppn = (uintptr_t)root >> 12;
    uintptr_t satp = (8ULL << 60) | ppn; // 8 == SATP_MODE_SV39
    asm volatile(
        "csrw satp, %0\n"
        "sfence.vma\n"
        :: "r"(satp) : "memory"
    );

    // Phase 2: Assert readback
    uintptr_t readback;
    asm volatile("csrr %0, satp" : "=r"(readback));
    if ((readback >> 60) != 8) {
        kernel_panic("SATP mode silently degraded after MMU activation");
    }
}

static void riscv_mmu_deactivate(void) {
    // Mode 0 is BARE
    asm volatile(
        "csrw satp, zero\n"
        "sfence.vma\n"
        ::: "memory"
    );
}

extern bool g_riscv_asid_supported;

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
    if (!g_riscv_asid_supported) {
        asm volatile("sfence.vma\n" ::: "memory");
    } else {
        asm volatile(
            "sfence.vma zero, %0\n"
            :: "r"(asid) : "memory"
        );
    }
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

#include "../../../../include/slab.h"

static prot_domain_t* riscv64_mmu_full_create(void) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return NULL;

    domain->mode = PROT_MODE_MMU_FULL;
    domain->backend_state = (void*)riscv64_mmu_ops.create_table(); // physical root pt
    return domain;
}

static void riscv64_mmu_full_destroy(prot_domain_t* domain) {
    if (!domain) return;
    riscv64_mmu_ops.destroy_table((phys_addr_t)domain->backend_state);
    kfree(domain);
}

static void riscv64_mmu_full_activate(prot_domain_t* domain) {
    if (!domain) return;

    // satp / Sv39 activation + sfence.vma hook
    riscv64_mmu_ops.activate((phys_addr_t)domain->backend_state);

    arch_caps_t caps = arch_get_caps();
    if (!arch_caps_test(caps, ARCH_CAP_ASID)) {
       riscv64_mmu_ops.tlb_flush_all();
    }
}

static int riscv64_mmu_full_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (!domain) return -1;
    return riscv64_mmu_ops.map((phys_addr_t)domain->backend_state, vaddr, paddr, size, flags);
}

static int riscv64_mmu_full_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (!domain) return -1;
    return riscv64_mmu_ops.unmap((phys_addr_t)domain->backend_state, vaddr, size, NULL);
}

static int riscv64_mmu_full_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (!domain) return -1;
    return riscv64_mmu_ops.protect((phys_addr_t)domain->backend_state, vaddr, size, flags);
}

static int riscv64_mmu_full_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (!domain) return -1;
    return riscv64_mmu_ops.query((phys_addr_t)domain->backend_state, vaddr, paddr, flags);
}

prot_domain_ops_t mmu_full_ops_riscv64 = {
    .create = riscv64_mmu_full_create,
    .destroy = riscv64_mmu_full_destroy,
    .activate = riscv64_mmu_full_activate,
    .map_region = riscv64_mmu_full_map_region,
    .unmap_region = riscv64_mmu_full_unmap_region,
    .protect_region = riscv64_mmu_full_protect_region,
    .query_region = riscv64_mmu_full_query_region,
};

void riscv_mmu_detect(mmu_ops_t *ops) {
    // Stub for runtime detection (e.g. probing satp for Sv48/Sv57)
    (void)ops;
}

int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr) {
    if (!active_hal_pt) hal_pt_init();

    int ret = active_hal_pt->unmap_page(root_table, vaddr, unmapped_paddr);

    if (ret == 0 && root_table == hal_get_satp()) {
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
        *flags = RISCV_PTE_V;
        if (pt_flags & HAL_PT_FLAG_WRITE) *flags |= CAP_RIGHT_WRITE;
        if (pt_flags & HAL_PT_FLAG_USER)  *flags |= PAGE_USER;
        // In original RISC-V implementation, DEVICE/NOCACHE mappings were preserved/read out manually.
        // We'll just preserve the basic ones for now as required by the old API wrapper.
    }

    return ret;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!active_hal_pt) hal_pt_init();

    uint32_t pt_flags = HAL_PT_FLAG_READ;
    if (flags & CAP_RIGHT_WRITE) pt_flags |= HAL_PT_FLAG_WRITE;
    if (flags & PAGE_USER)       pt_flags |= HAL_PT_FLAG_USER;

    if (flags & (1U << 30)) {
        pt_flags |= HAL_PT_FLAG_DEVICE;
    }
    if (flags & (1U << 31)) {
        pt_flags |= HAL_PT_FLAG_NOCACHE;
    }

    pt_flags |= HAL_PT_FLAG_EXEC;

    int ret = active_hal_pt->map_page(root_table, vaddr, paddr, pt_flags);

    if (ret == 0 && root_table == hal_get_satp()) {
        if (active_hal_tlb) active_hal_tlb->flush_page_local(vaddr);
    }

    return ret;
}
