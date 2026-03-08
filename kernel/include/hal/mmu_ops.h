#ifndef BHARAT_HAL_MMU_OPS_H
#define BHARAT_HAL_MMU_OPS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../include/mm.h"

// Universal flags (arch backends translate these)
typedef uint32_t mmu_flags_t;

#define MMU_READ     (1 << 0)
#define MMU_WRITE    (1 << 1)
#define MMU_EXEC     (1 << 2)
#define MMU_USER     (1 << 3)
#define MMU_GLOBAL   (1 << 4)
#define MMU_NOCACHE  (1 << 5)
#define MMU_DEVICE   (1 << 6)   // MMIO — strongly-ordered, no cache
#define MMU_HUGE     (1 << 7)   // request huge page if available
#define MMU_COW      (1 << 8)   // Copy-on-write flag

// The contract every arch must fulfill
typedef struct mmu_ops {
    // Lifecycle
    phys_addr_t (*create_table)(void);           // allocate root page table
    void        (*destroy_table)(phys_addr_t root);    // free entire table tree
    phys_addr_t (*clone_kernel)(phys_addr_t kernel_root); // copy kernel mappings

    // Core mapping
    int         (*map)(phys_addr_t root, virt_addr_t virt, phys_addr_t phys,
                       size_t size, mmu_flags_t flags);
    int         (*unmap)(phys_addr_t root, virt_addr_t virt, size_t size, phys_addr_t *unmapped_phys);
    int         (*protect)(phys_addr_t root, virt_addr_t virt, size_t size,
                           mmu_flags_t new_flags);

    // Query
    int         (*query)(phys_addr_t root, virt_addr_t virt,
                         phys_addr_t *phys_out, mmu_flags_t *flags_out);

    // Activation
    void        (*activate)(phys_addr_t root);         // load into hardware
    void        (*deactivate)(void);

    // TLB
    void        (*tlb_flush_page)(virt_addr_t virt);
    void        (*tlb_flush_all)(void);
    void        (*tlb_flush_asid)(uint16_t asid); // ARM/RISC-V have ASIDs

    // Capabilities (filled at init time)
    size_t      page_size;           // base page size (usually 4096)
    size_t      *huge_page_sizes;     // e.g. {2MB, 1GB, 0}
    uint8_t     asid_bits;           // 0 if no ASID support
    uint8_t     levels;              // page table depth
    bool        has_nx;              // no-execute support
    bool        has_user_kernel_split; // separate TTBRs (ARM64, RISC-V)
} mmu_ops_t;

// Global active MMU operations table
extern mmu_ops_t *active_mmu;

// Called early during boot to detect CPU/MMU features and initialize active_mmu
void arch_mmu_init(void);

#endif // BHARAT_HAL_MMU_OPS_H
