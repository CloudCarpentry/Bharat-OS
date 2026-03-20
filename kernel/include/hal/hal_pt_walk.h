#ifndef BHARAT_HAL_PT_WALK_H
#define BHARAT_HAL_PT_WALK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hal_pt.h"
#include "../../include/mm.h"

#ifdef __cplusplus
extern "C" {
#endif

// Generic Page Table Walker Result
typedef struct page_table_walk_result {
    phys_addr_t entry_pa;        // Physical address of the PTE itself
    void*       entry_va;        // Linear mapped virtual address of the PTE
    uint64_t    raw_value;       // Raw architecture-specific PTE value
    int         level;           // Level of the PT walk (e.g. 1=PT, 2=PD, 3=PDP, 4=PML4)
    bool        present;         // Is the entry present?
    bool        is_large;        // Is this a huge page mapping (2M, 1G)?
    phys_addr_t mapped_pa;       // The final physical address mapped (if present)
    uint32_t    flags;           // Parsed generic flags (HAL_PT_FLAG_*)
} page_table_walk_result_t;

// Opaque context for architecture-specific walkers
typedef struct pt_walk_ctx pt_walk_ctx_t;

// Operations for a generic PT walker, implemented by architecture
typedef struct pt_walk_ops {
    // Walks the page table to find the entry for vaddr
    // create: if true, allocate intermediate tables if missing
    // alloc_flags: generic permissions for newly created intermediate tables
    int (*walk)(phys_addr_t root_pt, virt_addr_t vaddr, bool create, uint32_t alloc_flags, page_table_walk_result_t *out_result);

    // Updates the PTE found by a previous walk
    int (*update)(const page_table_walk_result_t *result, phys_addr_t paddr, uint32_t new_flags);

    // Clears the PTE found by a previous walk
    int (*clear)(const page_table_walk_result_t *result, phys_addr_t *out_old_paddr);
} pt_walk_ops_t;

extern const pt_walk_ops_t *active_pt_walk_ops;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_HAL_PT_WALK_H
