#ifndef BHARAT_UAPI_EXECUTION_INTENT_H
#define BHARAT_UAPI_EXECUTION_INTENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BH_EXEC_INTENT_LATENCY = 1,
    BH_EXEC_INTENT_ENERGY,
    BH_EXEC_INTENT_SAFETY
} bh_exec_intent_class_t;

typedef enum {
    BH_ACCEL_DONT_CARE = 0,
    BH_ACCEL_PREFER,
    BH_ACCEL_REQUIRE,
    BH_ACCEL_AVOID
} bh_accel_preference_t;

typedef enum {
    BH_FALLBACK_DENY = 0,
    BH_FALLBACK_ALLOW
} bh_fallback_policy_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;

    uint64_t intent_id;

    uint32_t intent_class;
    uint32_t criticality;

    uint64_t latency_target_ns;
    uint64_t deadline_ns;

    uint32_t energy_policy;
    uint32_t accel_preference;

    uint32_t fallback_policy;
    uint32_t isolation_level;

    uint32_t flags;
    uint32_t reserved;
} bh_execution_intent_v1_t;

#define BHARAT_EXECUTION_INTENT_V1_VERSION 1

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_EXECUTION_INTENT_H
