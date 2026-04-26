#include <sched/sched.h>
#include <hal/hal.h>
#include "sched_internal.h"

bool sched_is_core_admissible(bh_thread_t *t, int cpu_id)
{
    if (!t) return false;

    // Explicitly allow idle threads on any core.
    if ((t->flags & BH_THREAD_FLAG_IDLE) != 0) {
        return true;
    }

    return (t->constraints.cpu_mask & (1u << cpu_id)) != 0;
}
