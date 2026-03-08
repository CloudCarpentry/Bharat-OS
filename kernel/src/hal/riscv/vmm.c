#include "../../../include/hal/vmm.h"
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
        l2_table->entries[vpn2] = ((new_l1 >> 12) << 10) | RISCV_PTE_V; // Directory entry has no R/W/X
    }

    pte_table_t* l1_table = (pte_table_t*)P2V((l2_table->entries[vpn2] >> 10) << 12);

    if ((l1_table->entries[vpn1] & RISCV_PTE_V) == 0) {
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pte_table_t* l0_ptr = (pte_table_t*)P2V(new_l0);
        for(int i=0; i<512; i++) l0_ptr->entries[i] = 0;
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | RISCV_PTE_V;
    }

    pte_table_t* l0_table = (pte_table_t*)P2V((l1_table->entries[vpn1] >> 10) << 12);

    // Create final PTE for L0 (page mapping)
    uint64_t pte_flags = convert_flags_to_riscv(flags);
    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

    return 0;
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

    return 0;
}
