#include "execution_planner.h"
#include <string.h>

bh_execution_plan_result_t
bh_execution_plan_build(
    const bh_execution_intent_v1_t *intent,
    const bharat_execution_config_t *execution_config,
    const bharat_hw_caps_t *hw_caps,
    const bh_accel_state_snapshot_t *accel,
    const bh_power_state_snapshot_t *power,
    const bh_thermal_state_snapshot_t *thermal,
    bh_execution_plan_v1_t *out)
{
    bh_execution_plan_result_t result = { .success = false, .status_code = -1 };

    if (!intent || !execution_config || !hw_caps || !accel || !power || !thermal || !out) {
        return result;
    }

    memset(out, 0, sizeof(*out));
    out->abi_version = BHARAT_EXECUTION_PLAN_V1_VERSION;
    out->struct_size = sizeof(bh_execution_plan_v1_t);
    out->intent_id = intent->intent_id;

    // Define symbolic constants here if they aren't available from headers
    // Memory classes (hypothetical)
    const uint32_t MEM_CLASS_SYSTEM = 0;
    const uint32_t MEM_CLASS_HMEM = 1;

    // Backend classes (hypothetical, aligning with backend_dispatch)
    const uint32_t BACKEND_CPU = 0;
    const uint32_t BACKEND_NPU = 1;
    const uint32_t BACKEND_SAFE = 2;

    switch (intent->intent_class) {
        case BH_EXEC_INTENT_LATENCY:
            // Prefer: RT-capable CPU, DEADLINE_RT scheduler, HMEM, NPU
            out->selected_cpu_mask = execution_config->realtime_cpu_mask;
            out->scheduler_class = BHARAT_SCHED_CLASS_DEADLINE_RT;
            out->memory_class = MEM_CLASS_HMEM;

            // Check NPU availability and capability
            if (accel->is_available) {
                if (accel->is_quarantined) {
                    out->backend_class = BACKEND_CPU;
                    out->decision_reason = BH_EXEC_REASON_ACCEL_QUARANTINED;
                } else if (!accel->capability_allowed) {
                    out->backend_class = BACKEND_CPU;
                    out->decision_reason = BH_EXEC_REASON_CAPABILITY_DENIED;
                } else {
                    out->backend_class = BACKEND_NPU;
                    out->decision_reason = BH_EXEC_REASON_SELECTED_PREFERRED;
                }
            } else {
                out->backend_class = BACKEND_CPU;
                out->decision_reason = BH_EXEC_REASON_ACCEL_UNAVAILABLE;
            }
            break;

        case BH_EXEC_INTENT_ENERGY:
            // Prefer: GP CPU, FAIR scheduler, SYSTEM memory, CPU backend
            out->selected_cpu_mask = execution_config->best_effort_cpu_mask;
            out->scheduler_class = BHARAT_SCHED_CLASS_FAIR;
            out->memory_class = MEM_CLASS_SYSTEM;
            out->backend_class = BACKEND_CPU;
            out->decision_reason = BH_EXEC_REASON_SELECTED_PREFERRED;
            break;

        case BH_EXEC_INTENT_SAFETY:
            // Prefer: ISOLATED CPU, safe backend, fallback required, strong isolation
            out->selected_cpu_mask = execution_config->isolated_cpu_mask;
            out->scheduler_class = BHARAT_SCHED_CLASS_SYSTEM; // Or whatever is safe
            out->memory_class = MEM_CLASS_SYSTEM;
            out->backend_class = BACKEND_SAFE;
            out->decision_reason = BH_EXEC_REASON_SELECTED_PREFERRED;
            break;

        default:
            out->decision_reason = BH_EXEC_REASON_NO_FEASIBLE_PLAN;
            return result;
    }

    // Check if the selected CPU mask is valid (non-zero and part of the system)
    if (out->selected_cpu_mask == 0) {
        if (intent->fallback_policy == BH_FALLBACK_ALLOW) {
            // Fallback to best effort if allowed
            out->selected_cpu_mask = execution_config->best_effort_cpu_mask;
            out->decision_reason = BH_EXEC_REASON_FALLBACK_SELECTED;
            if (out->selected_cpu_mask == 0) {
                 // Even fallback failed
                 out->decision_reason = BH_EXEC_REASON_NO_FEASIBLE_PLAN;
                 return result;
            }
        } else {
            out->decision_reason = BH_EXEC_REASON_NO_FEASIBLE_PLAN;
            return result;
        }
    }

    // Further fallback logic if intent required NPU but it wasn't chosen
    if (intent->accel_preference == BH_ACCEL_REQUIRE && out->backend_class != BACKEND_NPU) {
         if (intent->fallback_policy == BH_FALLBACK_DENY) {
             out->decision_reason = BH_EXEC_REASON_NO_FEASIBLE_PLAN;
             return result;
         }
    }

    result.success = true;
    result.status_code = 0;
    return result;
}
