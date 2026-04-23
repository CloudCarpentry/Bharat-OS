#include <assert.h>
#include <core/constraint_validate.h>

int main()
{
    bh_exec_constraints_k_t c = {0};
    c.cpu_mask = 1;

    assert(bh_validate_exec_constraints(&c) == BH_CONSTRAINT_OK);

    c.flags = 0x1 | 0x4;
    assert(bh_validate_exec_constraints(&c) == BH_CONSTRAINT_ERR_CONFLICT);

    return 0;
}
