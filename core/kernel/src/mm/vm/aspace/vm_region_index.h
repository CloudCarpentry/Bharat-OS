#ifndef BHARAT_MM_VM_REGION_INDEX_H
#define BHARAT_MM_VM_REGION_INDEX_H

#include <bharat/kernel/ds/bh_range_tree.h>
#include <mm/aspace.h>

/**
 * @file vm_region_index.h
 * @brief Canonical VM Region Lookup Adapter
 */

/**
 * @brief Canonical lookup for a VM region by virtual address.
 *
 * Internally uses the optimized range index (currently bh_range_tree_t sorted table).
 */
vm_region_t *vm_region_index_lookup(address_space_t *aspace, uintptr_t va);

/**
 * @brief Insert a region into the canonical index.
 */
kstatus_t vm_region_index_insert(address_space_t *aspace, vm_region_t *region);

/**
 * @brief Remove a region from the canonical index.
 */
kstatus_t vm_region_index_remove(address_space_t *aspace, vm_region_t *region);

#endif // BHARAT_MM_VM_REGION_INDEX_H
