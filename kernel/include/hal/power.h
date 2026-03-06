#ifndef BHARAT_HAL_POWER_H
#define BHARAT_HAL_POWER_H

#include <stdint.h>

/*
 * Bharat-OS Intelligent Power Management Interface
 * Primitive mechanisms exposed for the AI Governor to control dynamically.
 */

// P-State definition (Performance states)
// Lower values indicate higher performance and power draw
typedef uint32_t pstate_t;

// Set the CPU core P-state
// Returns 0 on success, <0 on failure
int hal_power_set_pstate(uint32_t core_id, pstate_t state);

// Completely gate the clock to a peripheral or capability block
// to eliminate dynamic power consumption.
// Returns 0 on success, <0 on failure
int hal_power_clock_gate(uint32_t device_id, int enable);

// Enter deeper sleep states (e.g. C-states in x86)
// Returns 0 on success, <0 on failure
int hal_power_enter_sleep_state(uint32_t cstate);

#endif // BHARAT_HAL_POWER_H
