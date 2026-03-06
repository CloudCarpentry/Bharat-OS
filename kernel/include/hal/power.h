#ifndef BHARAT_HAL_POWER_H
#define BHARAT_HAL_POWER_H

#include <stdint.h>
#include "../advanced/formal_verif.h"

/*
 * Bharat-OS Intelligent Power Management Interface
 * Primitive mechanisms exposed for the AI Governor to control dynamically.
 */

// P-State definition (Performance states)
// Lower values indicate higher performance and power draw
typedef uint32_t pstate_t;

typedef enum {
    POWER_DEVICE_CPU = 0,
    POWER_DEVICE_GPU = 1,
    POWER_DEVICE_NPU = 2,
    POWER_DEVICE_USB = 3,
    POWER_DEVICE_NIC = 4,
    POWER_DEVICE_OTHER = 255
} power_device_class_t;

typedef struct {
    uint32_t process_id;
    uint32_t workload_class; // e.g., compiler, game, inference, idle
    uint32_t latency_sensitivity;
    uint32_t expected_burst_us;
} dvfs_prediction_hint_t;

// Set the CPU core P-state
// Returns 0 on success, <0 on failure
int hal_power_set_pstate(uint32_t core_id, pstate_t state);

// Request predictive DVFS ramp-up/down from governor hints
// Returns 0 on success, <0 on failure
int hal_power_apply_dvfs_hint(uint32_t core_id, dvfs_prediction_hint_t* hint, capability_t* cap);

// Completely gate the clock to a peripheral or capability block
// to eliminate dynamic power consumption.
// Returns 0 on success, <0 on failure
int hal_power_clock_gate(uint32_t device_id, int enable);

// Power gate a hardware block when no active capability lease exists.
// Returns 0 on success, <0 on failure
int hal_power_set_device_power_state(uint32_t device_id, power_device_class_t class_id, int powered_on, capability_t* cap);

// Enter deeper sleep states (e.g. C-states in x86)
// Returns 0 on success, <0 on failure
int hal_power_enter_sleep_state(uint32_t cstate);

#endif // BHARAT_HAL_POWER_H
