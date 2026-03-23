#ifndef BHARAT_OS_CPUIDLE_H
#define BHARAT_OS_CPUIDLE_H

#include <stdint.h>
#include <stddef.h>

typedef struct cpuidle_state cpuidle_state_t;

typedef enum {
    IDLE_STATE_BUSY,
    IDLE_STATE_SHALLOW,
    IDLE_STATE_DEEP,
    IDLE_STATE_RETENTION,
    IDLE_STATE_SUSPEND
} idle_type_t;

struct cpuidle_state {
    uint32_t id;
    idle_type_t type;
    uint32_t target_residency_us;
    uint32_t exit_latency_us;
    uint32_t power_usage_mw;
    int (*enter)(struct cpuidle_state *state);
};

typedef struct cpuidle_device {
    int cpu_id;
    struct cpuidle_state *states;
    size_t state_count;
    uint64_t *residency_us;
} cpuidle_device_t;

int cpuidle_register_device(cpuidle_device_t *dev);
int cpuidle_enter_state(int cpu_id, uint32_t state_id);
int cpuidle_get_residency(int cpu_id, uint32_t state_id, uint64_t *out_res_us);

#endif /* BHARAT_OS_CPUIDLE_H */
