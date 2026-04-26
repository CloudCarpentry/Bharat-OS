#include "mm/user_range.h"
#include "trap.h"

// Forward declaration of the basic numeric check
extern int trap_user_range_valid(uintptr_t ptr, size_t len);

kstatus_t mm_user_range_validate_current(uintptr_t ptr, size_t len, uint32_t access) {
    (void)access; // Stage 3 will use this for permission checks

    // Stage 1.5: Numeric range check
    if (!trap_user_range_valid(ptr, len)) {
        return K_ERR_FAULT;
    }

#if defined(BHARAT_ENABLE_VMA_USERCOPY)
    // Stage 2: Actual VMA/page table validation would go here.
    // In strict mode (Stage 2 enabled), we fail closed if validation logic is missing.

    // TODO: Implement real_aspace_validate(sched_current_aspace(), ptr, len, access);
    return K_ERR_UNSUPPORTED;
#else
    // Fallback to Stage 1.5 (Numeric check only)
    return K_OK;
#endif
}
