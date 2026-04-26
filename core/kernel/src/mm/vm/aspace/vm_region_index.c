#include "vm_region_index.h"
#include <kernel/status.h>

/*
 * Initial adapter implementation.
 * For Phase 6, we use aspace_lookup_region as the backing implementation,
 * but provide the new canonical entry point.
 */

vm_region_t *vm_region_index_lookup(address_space_t *aspace, uintptr_t va) {
    /*
     * In the future, this will directly use a bh_range_tree_t
     * embedded in address_space_t. For now, it delegates to the
     * existing RB-tree based lookup.
     */
    return aspace_lookup_region(aspace, va);
}

kstatus_t vm_region_index_insert(address_space_t *aspace, vm_region_t *region) {
    /* Existing tree_insert is called by insert_region_sorted */
    return K_OK;
}

kstatus_t vm_region_index_remove(address_space_t *aspace, vm_region_t *region) {
    /* Existing tree_remove is called by aspace_region_detach */
    return K_OK;
}
