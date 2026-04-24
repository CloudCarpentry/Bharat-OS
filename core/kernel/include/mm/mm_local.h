#ifndef BHARAT_MM_LOCAL_H
#define BHARAT_MM_LOCAL_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"
#include "profile/profile.h"
#include "bharat/cpu_local.h"

#include <stdbool.h>

// Utility
static inline bool core_is_rt(void) {
    KernelExecutionProfile profile = get_kernel_execution_profile();

    if (profile == PROFILE_KERNEL_RT) {
        return true;
    } else if (profile == PROFILE_KERNEL_MIX) {
        // In MIX profile, some cores are RT and others are GP.
        // We query the specific core configuration.
        // For simplicity, we assume we can fetch this from cpu_local config if available
        // Currently fallback to returning false. In a full implementation, we'd check current_core_is_rt().
        return false;
    }

    // For GP profile or default fallback, it is not strictly RT.
    return false;
}

// Operations that touch only this core's state (no URPC, no blocking)

// Local core PT operations
int mm_local_map(address_space_t *as, virt_addr_t va, phys_addr_t pa, uint32_t flags);
int mm_local_unmap(address_space_t *as, virt_addr_t va);
int mm_local_protect(address_space_t *as, virt_addr_t va, uint32_t flags);


// Current core assertions
extern bool in_urpc_handler(void);
extern uint32_t current_core_id(void);
extern bool current_core_owns(address_space_t *as);

#endif // BHARAT_MM_LOCAL_H
