#ifndef BHARAT_UAPI_SYSTEM_POLICY_H
#define BHARAT_UAPI_SYSTEM_POLICY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bharat-OS Unified Policy Framework
 * Defines the core models for hardware-aware and profile-aware policy scaling.
 * Policy realization scales from static to dynamic based on capability tiers.
 */

typedef enum {
    BHARAT_POLICY_TIER_UNKNOWN = 0,
    BHARAT_POLICY_TIER_A_STATIC = 1,      /* Tiny / MPU-only: Boot-resolved static policy */
    BHARAT_POLICY_TIER_B_LIGHTWEIGHT = 2, /* Embedded / MMU-Lite: Event-driven thresholds */
    BHARAT_POLICY_TIER_C_DYNAMIC = 3      /* Rich / MMU-Full: Full dynamic policy & SLO loop */
} bharat_policy_tier_t;

/* Fault Domain Definitions */
typedef enum {
    BHARAT_FAULT_ACTION_IGNORE = 0,
    BHARAT_FAULT_ACTION_LOG = 1,
    BHARAT_FAULT_ACTION_RESTART_SVC = 2,
    BHARAT_FAULT_ACTION_ISOLATE = 3,
    BHARAT_FAULT_ACTION_SAFE_MODE = 4,
    BHARAT_FAULT_ACTION_HALT = 5
} bharat_fault_action_t;

typedef struct {
    uint32_t domain_id;
    bharat_fault_action_t default_action;
    uint32_t max_restarts;
    bool require_audit_log;
} bharat_fault_domain_policy_t;

/* Memory Classes */
typedef enum {
    BHARAT_MEM_CLASS_NORMAL = 0,
    BHARAT_MEM_CLASS_FAST = 1,
    BHARAT_MEM_CLASS_DMA = 2,
    BHARAT_MEM_CLASS_ISOLATED = 3
} bharat_memory_class_t;

/* Power & Thermal Policy Hooks */
typedef enum {
    BHARAT_POWER_STATE_FULL = 0,
    BHARAT_POWER_STATE_THROTTLED = 1,
    BHARAT_POWER_STATE_SUSPEND = 2,
    BHARAT_POWER_STATE_CRITICAL = 3
} bharat_power_state_t;

/* Unified Boot Policy Table (Tier A Primary) */
typedef struct {
    bharat_policy_tier_t active_tier;
    uint32_t allowed_subsystems_mask;
    bharat_power_state_t initial_power_state;
    uint32_t watchdog_timeout_ms;
} bharat_system_policy_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_SYSTEM_POLICY_H
