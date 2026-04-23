#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t flags;
    uint16_t latency_class;
    uint16_t energy_class;
    uint32_t cpu_mask;
} bh_exec_constraints_k_t;

typedef struct {
    uint16_t class_id;
    uint16_t reclaim_priority;
    uint16_t locality_domain;
    uint16_t flags;
} bh_mem_constraints_k_t;

bool bh_constraints_validate_exec(const bh_exec_constraints_k_t *c);
