#pragma once
#include <bharat/constraints.h>

typedef enum {
    BH_CONSTRAINT_OK = 0,
    BH_CONSTRAINT_ERR_INVALID,
    BH_CONSTRAINT_ERR_CONFLICT,
    BH_CONSTRAINT_ERR_UNSUPPORTED
} bh_constraint_status_t;

bh_constraint_status_t
bh_validate_exec_constraints(const bh_exec_constraints_k_t *c);
