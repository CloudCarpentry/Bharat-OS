#include <sched/sched.h>
#include <hal/hal.h>
#include "sched_internal.h"

bool sched_is_core_admissible(bh_thread_t *t, int cpu_id)
{
    if (!t) return false;

    // Use a simpler check rather than fully dereferencing internal rq state
    // since g_active_core_count is static to sched.c.
    // The idle thread has thread_id 0 implicitly or we can just rely on the sched core loops.
    // If it's the idle thread, its mask is typically 0xFFFFFFFF anyway, but let's be safe.
    if (t->thread_id == 0) return true; // Boot/Idle thread

    return (t->constraints.cpu_mask & (1u << cpu_id)) != 0;
}
