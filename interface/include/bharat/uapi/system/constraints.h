/*
 * Transitional UAPI placeholder.
 * This header exists to unblock current build wiring.
 */
#ifndef BHARAT_UAPI_SYSTEM_CONSTRAINTS_H
#define BHARAT_UAPI_SYSTEM_CONSTRAINTS_H

#include <stdint.h>

typedef struct bh_exec_constraints {
    uint32_t flags;
    uint32_t cpu_mask_hint;
    uint32_t latency_target_us;
    uint32_t energy_budget_uw;
    uint32_t priority_class;
    uint32_t memory_budget_kb;
    uint32_t isolation_domain;
} bh_exec_constraints_t;

#endif // BHARAT_UAPI_SYSTEM_CONSTRAINTS_H
