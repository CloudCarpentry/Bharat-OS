#ifndef BHARAT_HAL_MPU_H
#define BHARAT_HAL_MPU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../mm.h"

// Hardware specific MPU constraints and limits
typedef struct hal_mpu_caps {
    uint32_t max_regions;
    bool supports_subregions;
    bool requires_power_of_two_size;
    uint32_t min_region_alignment;
} hal_mpu_caps_t;

// Multikernel MPU Configuration API
// Note: Like TLB, MPU state is localized to a core.
void hal_mpu_init(void);
const hal_mpu_caps_t* hal_mpu_get_caps(void);

int hal_mpu_program_region(uint32_t region_id, phys_addr_t base, size_t size, uint32_t flags);
int hal_mpu_disable_region(uint32_t region_id);

void hal_mpu_activate_local(void);
void hal_mpu_deactivate_local(void);

#endif // BHARAT_HAL_MPU_H
