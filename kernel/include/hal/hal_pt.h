#ifndef BHARAT_HAL_PT_H
#define BHARAT_HAL_PT_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"

// Generic Page Table Flags
#define HAL_PT_FLAG_READ    (1 << 0)
#define HAL_PT_FLAG_WRITE   (1 << 1)
#define HAL_PT_FLAG_EXEC    (1 << 2)
#define HAL_PT_FLAG_USER    (1 << 3)
#define HAL_PT_FLAG_GLOBAL  (1 << 4)
#define HAL_PT_FLAG_NOCACHE (1 << 5)
#define HAL_PT_FLAG_DEVICE  (1 << 6)
#define HAL_PT_FLAG_COW     (1 << 7)

// Architecture-neutral Page Table Manager API
typedef struct hal_pt_ops {
    // Lifecycle
    phys_addr_t (*create_address_space)(phys_addr_t kernel_root_table);
    void        (*destroy_address_space)(phys_addr_t root_pt);

    // Mapping
    int         (*map_page)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags);
    int         (*unmap_page)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr);
    int         (*protect_page)(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags);

    // Query
    int         (*query_page)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags);
} hal_pt_ops_t;

extern hal_pt_ops_t *active_hal_pt;

void hal_pt_init(void);

#endif // BHARAT_HAL_PT_H
