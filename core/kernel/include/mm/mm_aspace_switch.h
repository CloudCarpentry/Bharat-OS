#ifndef BHARAT_MM_ASPACE_SWITCH_H
#define BHARAT_MM_ASPACE_SWITCH_H

#include <stdint.h>
#include "aspace.h"

#ifdef __cplusplus
extern "C" {
#endif

void mm_switch_active_aspace(uint32_t core_id, address_space_t *prev_as, address_space_t *next_as);

void vm_debug_validate_active_tracking(void);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_ASPACE_SWITCH_H
