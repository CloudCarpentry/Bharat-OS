#include <sched/sched.h>
#include <sched/cpu_partition.h>
#include <hal/hal.h>
#include "sched_internal.h"

static bharat_sched_class_mask_t sched_class_mask_from_thread(
    const bh_thread_t *thread, uint32_t core_id) {
    if (!thread) return BHARAT_SCHED_CLASS_NONE;

    if (thread->flags & BH_THREAD_FLAG_IDLE) {
        return BHARAT_SCHED_CLASS_IDLE;
    }

    switch (sched_policy_for_core(core_id)) {
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
    if (!t || cpu_id < 0 || cpu_id >= 32) return false;

    // Explicitly allow idle threads on any core.
    if ((t->flags & BH_THREAD_FLAG_IDLE) != 0) {
        return true;
    }

    // Harden: verify class placement against CPU partition rules
    bharat_sched_class_mask_t class_mask =
        sched_class_mask_from_thread(t, (uint32_t)cpu_id);
    if (!cpu_partition_allows_class(cpu_id, class_mask)) {
        return false;
    }

    uint32_t cpu_bit = 1U << (uint32_t)cpu_id;
    if ((t->affinity_mask & cpu_bit) == 0U) {
        return false;
    }

    return (t->constraints.cpu_mask & cpu_bit) != 0U;
}

bh_thread_t *sched_validate_picked_candidate(bh_thread_t *candidate,
                                             bh_thread_t *idle,
                                             uint32_t core_id)
{
    if (!candidate) {
        return idle;
    }

    /* Ownership is the first dispatch gate; foreign entities fail closed. */
    if (__atomic_load_n(&candidate->owner_cpu, __ATOMIC_ACQUIRE) != core_id) {
        return idle;
    }

    /* This checks class/partition before affinity and dynamic constraints. */
    if (!sched_is_core_admissible(candidate, (int)core_id)) {
        return idle;
    }

    return candidate;
}

void sched_account_context_switch(sched_rq_t *rq, bh_thread_t *next)
{
    if (!rq || !next) {
        return;
    }

    next->context_switch_count++;
    rq->context_switches++;
}
