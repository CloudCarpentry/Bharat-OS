#include <core/constraint_validate.h>

bh_constraint_status_t
bh_validate_exec_constraints(const bh_exec_constraints_k_t *c)
{
    if (!c) return BH_CONSTRAINT_ERR_INVALID;

    if ((c->flags & 0x1) && (c->flags & 0x4)) {
        // latency + energy conflict (MVP rule)
        return BH_CONSTRAINT_ERR_CONFLICT;
    }

    if (c->cpu_mask == 0)
        return BH_CONSTRAINT_ERR_INVALID;

    return BH_CONSTRAINT_OK;
}
