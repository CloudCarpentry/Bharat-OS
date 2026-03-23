#ifndef BHARAT_MM_LOCAL_H
#define BHARAT_MM_LOCAL_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"
#include "profile/profile.h"

#include <stdbool.h>

// Utility
static inline bool core_is_rt(void) {
    MemoryModel model = get_memory_model();
    // Assuming MEM_MODEL_MPU/FLAT align with RT profiles, or explicitly check profile.
    // In a full implementation, this should query the actual core profile configuration.
    // For now, if the system is configured for RTOS, return true.
#ifdef Profile_RTOS
    return true;
#else
    return (model == MEM_MODEL_MPU || model == MEM_MODEL_FLAT);
#endif
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
