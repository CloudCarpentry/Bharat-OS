#include "../../include/mm/tlb.h"
#include "../../include/mm/tlb_internal.h"
#include "../../include/hal/hal_tlb.h"

// Basic ASID/PCID abstraction stub
int tlb_asid_alloc(vm_aspace_t *as, uint64_t *asid_out) {
    if (!as || !asid_out) return -1;
    // Simple generation-based ASID allocation
    *asid_out = (as->object_id & 0xFFFF);
    return 0;
}

int tlb_asid_free(vm_aspace_t *as, uint64_t asid) {
    (void)as;
    (void)asid;
    return 0;
}
