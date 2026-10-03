#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../../../../core/services/device/accelmgr/execution_planner.h"
#include <bharat/uapi/system/execution_mode.h>
#include <uapi/capability/hw_caps.h>

#define BACKEND_CPU 0
#define BACKEND_NPU 1
#define BACKEND_SAFE 2

#define MEM_CLASS_SYSTEM 0
#define MEM_CLASS_HMEM 1

static void test_planner_latency_normal(void) {
    bh_execution_intent_v1_t intent = {
        .intent_class = BH_EXEC_INTENT_LATENCY,
        .accel_preference = BH_ACCEL_PREFER,
        .fallback_policy = BH_FALLBACK_ALLOW
    };

    bharat_execution_config_t config = {
        .realtime_cpu_mask = 0x04,
        .best_effort_cpu_mask = 0x02,
        .isolated_cpu_mask = 0x01
    };

    bharat_hw_caps_t hw_caps = {0};

    bh_accel_state_snapshot_t accel = {
        .is_available = true,
        .is_quarantined = false,
        .capability_allowed = true
    };

    bh_power_state_snapshot_t power = {0};
    bh_thermal_state_snapshot_t thermal = {0};

    bh_execution_plan_v1_t plan;
    bh_execution_plan_result_t result = bh_execution_plan_build(
        &intent, &config, &hw_caps, &accel, &power, &thermal, &plan);

    assert(result.success == true);
    assert(plan.selected_cpu_mask == 0x04);
    assert(plan.scheduler_class == BHARAT_SCHED_CLASS_DEADLINE_RT);
    assert(plan.memory_class == MEM_CLASS_HMEM);
    assert(plan.backend_class == BACKEND_NPU);
    assert(plan.decision_reason == BH_EXEC_REASON_SELECTED_PREFERRED);
    printf("test_planner_latency_normal passed\n");
}

static void test_planner_energy_normal(void) {
    bh_execution_intent_v1_t intent = {
        .intent_class = BH_EXEC_INTENT_ENERGY,
        .accel_preference = BH_ACCEL_DONT_CARE,
        .fallback_policy = BH_FALLBACK_ALLOW
    };

    bharat_execution_config_t config = {
        .realtime_cpu_mask = 0x04,
        .best_effort_cpu_mask = 0x02,
        .isolated_cpu_mask = 0x01
    };

    bharat_hw_caps_t hw_caps = {0};
    bh_accel_state_snapshot_t accel = { .is_available = true };
    bh_power_state_snapshot_t power = {0};
    bh_thermal_state_snapshot_t thermal = {0};
    bh_execution_plan_v1_t plan;

    bh_execution_plan_result_t result = bh_execution_plan_build(
        &intent, &config, &hw_caps, &accel, &power, &thermal, &plan);

    assert(result.success == true);
    assert(plan.selected_cpu_mask == 0x02);
    assert(plan.scheduler_class == BHARAT_SCHED_CLASS_FAIR);
    assert(plan.memory_class == MEM_CLASS_SYSTEM);
    assert(plan.backend_class == BACKEND_CPU);
    assert(plan.decision_reason == BH_EXEC_REASON_SELECTED_PREFERRED);
    printf("test_planner_energy_normal passed\n");
}

static void test_planner_safety_normal(void) {
    bh_execution_intent_v1_t intent = {
        .intent_class = BH_EXEC_INTENT_SAFETY,
        .accel_preference = BH_ACCEL_AVOID,
        .fallback_policy = BH_FALLBACK_ALLOW
    };

    bharat_execution_config_t config = {
        .realtime_cpu_mask = 0x04,
        .best_effort_cpu_mask = 0x02,
        .isolated_cpu_mask = 0x01
    };

    bharat_hw_caps_t hw_caps = {0};
    bh_accel_state_snapshot_t accel = { .is_available = true };
    bh_power_state_snapshot_t power = {0};
    bh_thermal_state_snapshot_t thermal = {0};
    bh_execution_plan_v1_t plan;

    bh_execution_plan_result_t result = bh_execution_plan_build(
        &intent, &config, &hw_caps, &accel, &power, &thermal, &plan);

    assert(result.success == true);
    assert(plan.selected_cpu_mask == 0x01);
    assert(plan.backend_class == BACKEND_SAFE);
    assert(plan.decision_reason == BH_EXEC_REASON_SELECTED_PREFERRED);
    printf("test_planner_safety_normal passed\n");
}

static void test_planner_npu_quarantined(void) {
    bh_execution_intent_v1_t intent = {
        .intent_class = BH_EXEC_INTENT_LATENCY,
        .accel_preference = BH_ACCEL_PREFER,
        .fallback_policy = BH_FALLBACK_ALLOW
    };

    bharat_execution_config_t config = {
        .realtime_cpu_mask = 0x04,
    };

    bharat_hw_caps_t hw_caps = {0};
    bh_accel_state_snapshot_t accel = {
        .is_available = true,
        .is_quarantined = true,
        .capability_allowed = true
    };
    bh_power_state_snapshot_t power = {0};
    bh_thermal_state_snapshot_t thermal = {0};
    bh_execution_plan_v1_t plan;

    bh_execution_plan_result_t result = bh_execution_plan_build(
        &intent, &config, &hw_caps, &accel, &power, &thermal, &plan);

    assert(result.success == true);
    assert(plan.backend_class == BACKEND_CPU);
    assert(plan.decision_reason == BH_EXEC_REASON_ACCEL_QUARANTINED);
    printf("test_planner_npu_quarantined passed\n");
}

static void test_planner_npu_capability_denied(void) {
    bh_execution_intent_v1_t intent = {
        .intent_class = BH_EXEC_INTENT_LATENCY,
        .accel_preference = BH_ACCEL_PREFER,
        .fallback_policy = BH_FALLBACK_ALLOW
    };

    bharat_execution_config_t config = {
        .realtime_cpu_mask = 0x04,
    };

    bharat_hw_caps_t hw_caps = {0};
    bh_accel_state_snapshot_t accel = {
        .is_available = true,
        .is_quarantined = false,
        .capability_allowed = false
    };
    bh_power_state_snapshot_t power = {0};
    bh_thermal_state_snapshot_t thermal = {0};
    bh_execution_plan_v1_t plan;

    bh_execution_plan_result_t result = bh_execution_plan_build(
        &intent, &config, &hw_caps, &accel, &power, &thermal, &plan);

    assert(result.success == true);
    assert(plan.backend_class == BACKEND_CPU);
    assert(plan.decision_reason == BH_EXEC_REASON_CAPABILITY_DENIED);
    printf("test_planner_npu_capability_denied passed\n");
}

static void test_planner_no_feasible_plan(void) {
    bh_execution_intent_v1_t intent = {
        .intent_class = BH_EXEC_INTENT_LATENCY,
        .accel_preference = BH_ACCEL_REQUIRE,
        .fallback_policy = BH_FALLBACK_DENY
    };

    bharat_execution_config_t config = {
        .realtime_cpu_mask = 0x04,
    };

    bharat_hw_caps_t hw_caps = {0};
    bh_accel_state_snapshot_t accel = {
        .is_available = true,
        .is_quarantined = true,
        .capability_allowed = true
    };
    bh_power_state_snapshot_t power = {0};
    bh_thermal_state_snapshot_t thermal = {0};
    bh_execution_plan_v1_t plan;

    bh_execution_plan_result_t result = bh_execution_plan_build(
        &intent, &config, &hw_caps, &accel, &power, &thermal, &plan);

    assert(result.success == false);
    assert(plan.decision_reason == BH_EXEC_REASON_NO_FEASIBLE_PLAN);
    printf("test_planner_no_feasible_plan passed\n");
}

int main(void) {
    test_planner_latency_normal();
    test_planner_energy_normal();
    test_planner_safety_normal();
    test_planner_npu_quarantined();
    test_planner_npu_capability_denied();
    test_planner_no_feasible_plan();

    printf("\nAll planner tests passed!\n");
    return 0;
}
