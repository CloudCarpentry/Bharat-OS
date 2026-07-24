#ifndef BHARAT_VEHICLE_SIM_H
#define BHARAT_VEHICLE_SIM_H

#include <bharat/vehicle/bh_vehicle_event.h>
#include <stdbool.h>

void bh_vehicle_sim_init(uint64_t seed);
bool bh_vehicle_sim_next_event(bh_vehicle_event_t *out);
void bh_vehicle_sim_set_fault_mode(uint32_t fault_flags);

#endif // BHARAT_VEHICLE_SIM_H
