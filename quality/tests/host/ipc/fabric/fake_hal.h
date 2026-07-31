#ifndef BH_FAKE_HAL_H
#define BH_FAKE_HAL_H

#include <stdint.h>
#include <stdbool.h>

void fake_hal_set_cpu_id(uint32_t cpu_id);
void fake_hal_set_ticks(uint64_t ticks);
void fake_hal_ticks_add(uint64_t delta);

uint32_t fake_hal_get_ipi_count(uint32_t target_cpu);
void fake_hal_reset_ipi_counters(void);

#endif // BH_FAKE_HAL_H
