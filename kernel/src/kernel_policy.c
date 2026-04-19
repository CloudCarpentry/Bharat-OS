#include <bharat/uapi/system/policy.h>
#include <bharat/uapi/system/slo.h>

static bharat_system_policy_t g_active_system_policy = {
    .active_tier = BHARAT_POLICY_TIER_A_STATIC,
    .allowed_subsystems_mask = 0xFFFFFFFF, // allow all by default until resolved
    .initial_power_state = BHARAT_POWER_STATE_FULL,
    .watchdog_timeout_ms = 5000
};

static bharat_slo_gates_t g_active_slo_gates = {
    .max_dispatch_latency_us = 1000,
    .max_runqueue_depth = 128,
    .max_ipc_queue_depth = 256,
    .mem_pressure_threshold_pct = 90,
    .watchdog_miss_threshold = 3
};

void kernel_set_system_policy(const bharat_system_policy_t *policy) {
    if (policy) {
        g_active_system_policy = *policy;
    }
}

const bharat_system_policy_t* kernel_get_system_policy(void) {
    return &g_active_system_policy;
}

void kernel_set_slo_gates(const bharat_slo_gates_t *gates) {
    if (gates) {
        g_active_slo_gates = *gates;
    }
}

const bharat_slo_gates_t* kernel_get_slo_gates(void) {
    return &g_active_slo_gates;
}
