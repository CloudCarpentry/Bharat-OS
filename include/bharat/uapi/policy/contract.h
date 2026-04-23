#ifndef BHARAT_UAPI_POLICY_CONTRACT_H
#define BHARAT_UAPI_POLICY_CONTRACT_H

#include <stdint.h>

#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/policy/status.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file contract.h
 * @brief Unified policy-plane UAPI contract shared by policymgr and peer managers.
 */

#define POLICYMGR_SERVICE_ID 0x00010010u

enum {
    BHARAT_POLICY_CONTRACT_VERSION_V1 = 1u,
};

typedef enum {
    BHARAT_POLICY_TIER_STATIC = 1u,      /* Tier A: build/boot-resolved policy */
    BHARAT_POLICY_TIER_LIGHTWEIGHT = 2u, /* Tier B: event-driven lightweight runtime policy */
    BHARAT_POLICY_TIER_FULL = 3u,        /* Tier C: full dynamic runtime policy plane */
} bharat_policy_realization_tier_t;

typedef enum {
    BHARAT_POLICY_PROTECTION_UNKNOWN = 0u,
    BHARAT_POLICY_PROTECTION_MPU_ONLY = 1u,
    BHARAT_POLICY_PROTECTION_MMU_LITE = 2u,
    BHARAT_POLICY_PROTECTION_MMU_FULL = 3u,
} bharat_policy_protection_model_t;

typedef enum {
    BHARAT_POLICY_OP_RESOLVE_BOOT_SNAPSHOT = 1u,
    BHARAT_POLICY_OP_GET_ACTIVE_SNAPSHOT = 2u,
    BHARAT_POLICY_OP_APPLY_RUNTIME_UPDATE = 3u,
    BHARAT_POLICY_OP_REPORT_SLO_SAMPLE = 4u,
    BHARAT_POLICY_OP_EVAL_SLO_GATES = 5u,
    BHARAT_POLICY_OP_GET_ENFORCEMENT = 6u,
} bharat_policy_opcode_t;

enum {
    BHARAT_POLICY_SUBSYS_SCHED      = (1u << 0),
    BHARAT_POLICY_SUBSYS_IPC        = (1u << 1),
    BHARAT_POLICY_SUBSYS_MEMORY     = (1u << 2),
    BHARAT_POLICY_SUBSYS_POWER      = (1u << 3),
    BHARAT_POLICY_SUBSYS_TELEMETRY  = (1u << 4),
    BHARAT_POLICY_SUBSYS_SAFETY     = (1u << 5),
    BHARAT_POLICY_SUBSYS_DEVICE     = (1u << 6),
    BHARAT_POLICY_SUBSYS_NETWORK    = (1u << 7),
    BHARAT_POLICY_SUBSYS_STORAGE    = (1u << 8),
};

enum {
    BHARAT_POLICY_SLO_DOMAIN_SCHED_LATENCY = 1u,
    BHARAT_POLICY_SLO_DOMAIN_IPC_QUEUE = 2u,
    BHARAT_POLICY_SLO_DOMAIN_MEMORY_PRESSURE = 3u,
    BHARAT_POLICY_SLO_DOMAIN_WATCHDOG_HEARTBEAT = 4u,
    BHARAT_POLICY_SLO_DOMAIN_THERMAL = 5u,
};

enum {
    BHARAT_POLICY_ACTION_NONE = 0u,
    BHARAT_POLICY_ACTION_THROTTLE_BEST_EFFORT = 1u,
    BHARAT_POLICY_ACTION_REJECT_NEW_WORK = 2u,
    BHARAT_POLICY_ACTION_RECLAIM_MEMORY = 3u,
    BHARAT_POLICY_ACTION_ENTER_SAFE_MODE = 4u,
    BHARAT_POLICY_ACTION_RESTART_FAULT_DOMAIN = 5u,
};

typedef struct {
    uint32_t contract_version;
    uint32_t profile_mask;
    uint32_t hw_capability_flags;
    uint32_t protection_model;
    uint32_t realization_tier;
    uint32_t enabled_subsystems;
    bharat_policy_status_t status;
    uint32_t generation;
    uint64_t effective_since_ticks;
} bharat_policy_snapshot_t;

typedef struct {
    uint32_t requested_profile_mask;
    uint32_t hw_capability_flags;
    uint32_t protection_model;
    uint32_t requested_subsystems;
} bharat_policy_req_resolve_boot_snapshot_t;

typedef struct {
    bharat_policy_snapshot_t snapshot;
} bharat_policy_resp_boot_snapshot_t;

typedef struct {
    uint32_t expected_generation;
    bharat_policy_snapshot_t proposed_snapshot;
} bharat_policy_req_apply_runtime_update_t;

typedef struct {
    bharat_policy_status_t status;
    uint32_t new_generation;
} bharat_policy_resp_apply_runtime_update_t;

typedef struct {
    uint32_t slo_domain;
    uint32_t signal_id;
    uint64_t observed_value;
    uint64_t target_value;
    uint64_t sample_timestamp_ticks;
} bharat_policy_req_report_slo_sample_t;

typedef struct {
    uint32_t slo_domain;
    uint32_t evaluation_window_ms;
    uint32_t max_breach_count;
    uint32_t reserved;
} bharat_policy_req_eval_slo_gate_t;

typedef struct {
    bharat_policy_status_t gate_status;
    uint32_t breach_count;
    uint32_t enforcement_action;
    uint32_t enforcement_scope_subsystems;
} bharat_policy_resp_eval_slo_gate_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_POLICY_CONTRACT_H */
