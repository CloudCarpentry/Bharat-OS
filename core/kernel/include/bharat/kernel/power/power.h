#ifndef BHARAT_KERNEL_POWER_POWER_H
#define BHARAT_KERNEL_POWER_POWER_H

#include <stdint.h>

void bh_power_signal_event(uint32_t event_type);
void bh_thermal_signal_trip(uint32_t zone_id, int32_t temp);

#endif
