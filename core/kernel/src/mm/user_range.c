#include "mm/user_range.h"
#include "trap.h"

// Forward declaration of the basic numeric check
extern int trap_user_range_valid(uintptr_t ptr, size_t len);

#include "mm/aspace.h"
#include "sched/sched.h"

kstatus_t mm_user_range_validate_current(uintptr_t ptr, size_t len, uint32_t access) {
    // Stage 1.5: Numeric range check
    if (!trap_user_range_valid(ptr, len)) {
        return K_ERR_FAULT;
    }

#if defined(BHARAT_ENABLE_VMA_USERCOPY)
    // Stage 2A: Authoritative VMA/region validation
    address_space_t *aspace = sched_current_aspace();
    if (!aspace) return K_ERR_DENIED;

    uintptr_t curr = ptr;
    uintptr_t end = ptr + len;

    while (curr < end) {
        vm_region_t *region = aspace_lookup_region(aspace, curr);
        if (!region) return K_ERR_FAULT;

        // Check permissions
        if (access & BH_USER_ACCESS_READ) {
            if (!(region->prot & PROT_READ)) return K_ERR_DENIED;
        }
        if (access & BH_USER_ACCESS_WRITE) {
            if (!(region->prot & PROT_WRITE)) return K_ERR_DENIED;
        }
        if (access & BH_USER_ACCESS_EXECUTE) {
            if (!(region->prot & PROT_EXEC)) return K_ERR_DENIED;
        }

        // Move to next region or end of requested range
        uintptr_t region_end = region->base + region->length;
        if (region_end >= end) {
            break;
        }
        curr = region_end;
    }

    return K_OK;
#else
    (void)access;
    // Fallback to Stage 1.5 (Numeric check only)
    return K_OK;
#endif
}
