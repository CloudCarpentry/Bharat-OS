#include <bharat/constraints.h>

bool bh_constraints_validate_exec(const bh_exec_constraints_k_t *c)
{
    if (!c) return false;

    if (c->cpu_mask == 0)
        return false;

    return true;
}
