#pragma once
#include <stdint.h>

#define BH_CONSTRAINT_VERSION 1

typedef enum {
    BH_INTENT_LATENCY      = 1u << 0,
    BH_INTENT_THROUGHPUT   = 1u << 1,
    BH_INTENT_ENERGY       = 1u << 2,
    BH_INTENT_ISOLATION    = 1u << 3,
    BH_INTENT_MEMORY_BOUND = 1u << 4,
    BH_INTENT_PIPELINE     = 1u << 5,
} bh_intent_flags_t;

typedef struct {
    uint32_t flags;
    uint32_t priority_class;
    uint32_t latency_target_us;
    uint32_t energy_budget_uw;
    uint32_t memory_budget_kb;
    uint32_t isolation_domain;
    uint32_t cpu_mask_hint;
} bh_exec_constraints_t;

typedef struct {
    uint32_t class_id;
    uint32_t lifetime_ms;
    uint32_t locality;
    uint32_t reclaim_priority;
    uint32_t reliability;
    uint32_t zero_copy:1;
    uint32_t pinned:1;
    uint32_t hugepage:1;
} bh_mem_constraints_t;
