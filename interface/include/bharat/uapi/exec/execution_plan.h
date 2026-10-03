#ifndef BHARAT_UAPI_EXECUTION_PLAN_H
#define BHARAT_UAPI_EXECUTION_PLAN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BH_EXEC_REASON_SELECTED_PREFERRED = 1,
    BH_EXEC_REASON_ACCEL_UNAVAILABLE,
    BH_EXEC_REASON_ACCEL_QUARANTINED,
    BH_EXEC_REASON_CAPABILITY_DENIED,
    BH_EXEC_REASON_THERMAL_LIMIT,
    BH_EXEC_REASON_POWER_POLICY,
    BH_EXEC_REASON_SCHED_NOT_ADMISSIBLE,
    BH_EXEC_REASON_MEMORY_UNAVAILABLE,
    BH_EXEC_REASON_FALLBACK_SELECTED,
    BH_EXEC_REASON_NO_FEASIBLE_PLAN
} bh_execution_decision_reason_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;

    uint64_t intent_id;
    uint64_t plan_generation;

    uint32_t selected_cpu_mask;
    uint32_t scheduler_class;

    uint32_t memory_class;
    uint32_t backend_class;

    uint64_t accelerator_handle;

    uint32_t fallback_backend;
    uint32_t decision_reason;

    uint32_t flags;
    uint32_t reserved;
} bh_execution_plan_v1_t;

#define BHARAT_EXECUTION_PLAN_V1_VERSION 1

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_EXECUTION_PLAN_H
