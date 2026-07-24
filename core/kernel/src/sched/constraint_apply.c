#include <sched/sched.h>
#include <sched/cpu_partition.h>
#include <hal/hal.h>
#include "sched_internal.h"

static bharat_sched_class_mask_t sched_class_mask_from_thread(const bh_thread_t *thread) {
    if (!thread) return BHARAT_SCHED_CLASS_NONE;

    if (thread->flags & BH_THREAD_FLAG_IDLE) {
        return BHARAT_SCHED_CLASS_IDLE;
    }

    extern sched_policy_t g_policy;
    switch (g_policy) {
        case SCHED_POLICY_EDF:
        case SCHED_POLICY_RMS:
            return BHARAT_SCHED_CLASS_DEADLINE_RT;
        case SCHED_POLICY_ROUND_ROBIN:
        case SCHED_POLICY_PRIORITY:
            // This is a bit ambiguous as it could be SYSTEM or FIFO_RT
            // But usually priority-based in Bharat-OS core is used for SYSTEM/RT
            if (thread->priority >= 24) return BHARAT_SCHED_CLASS_SYSTEM;
            return BHARAT_SCHED_CLASS_FIFO_RT;
        case SCHED_POLICY_CLOUD_FAIR:
        default:
            return BHARAT_SCHED_CLASS_FAIR;
    }
}

bool sched_is_core_admissible(bh_thread_t *t, int cpu_id)
{
    if (!t) return false;

    // Explicitly allow idle threads on any core.
    if ((t->flags & BH_THREAD_FLAG_IDLE) != 0) {
        return true;
    }

    // Harden: verify class placement against CPU partition rules
    bharat_sched_class_mask_t class_mask = sched_class_mask_from_thread(t);
    if (!cpu_partition_allows_class(cpu_id, class_mask)) {
        return false;
    }

    return (t->constraints.cpu_mask & (1u << cpu_id)) != 0;
}
