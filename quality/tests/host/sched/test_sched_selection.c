#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sched/sched.h>
#include <sched/cpu_partition.h>
#include "sched_internal.h"

static sched_policy_t g_core_policy[2] = {
    SCHED_POLICY_CLOUD_FAIR,
    SCHED_POLICY_EDF,
};

sched_policy_t sched_policy_for_core(uint32_t core_id)
{
    return core_id < 2U ? g_core_policy[core_id] : SCHED_POLICY_PRIORITY;
}

static bool g_partition_allows;
static bharat_sched_class_mask_t g_last_class[2];

bool cpu_partition_allows_class(uint32_t cpu_id,
                                bharat_sched_class_mask_t class_mask)
{
    if (cpu_id < 2U) {
        g_last_class[cpu_id] = class_mask;
    }
    return g_partition_allows;
}

static void init_candidate(bh_thread_t *thread, uint32_t owner_cpu)
{
    memset(thread, 0, sizeof(*thread));
    thread->owner_cpu = owner_cpu;
    thread->affinity_mask = UINT32_MAX;
    thread->constraints.cpu_mask = UINT32_MAX;
}

static void test_inadmissible_task_is_never_selected(void)
{
    bh_thread_t candidate;
    bh_thread_t idle;
    init_candidate(&candidate, 0U);
    init_candidate(&idle, 0U);
    idle.flags = BH_THREAD_FLAG_IDLE;
    g_partition_allows = false;

    assert(sched_validate_picked_candidate(&candidate, &idle, 0U) == &idle);
}

static void test_affinity_mismatch_fails_closed(void)
{
    bh_thread_t candidate;
    bh_thread_t idle;
    init_candidate(&candidate, 0U);
    init_candidate(&idle, 0U);
    candidate.affinity_mask = 1U << 1U;
    g_partition_allows = true;

    assert(sched_validate_picked_candidate(&candidate, &idle, 0U) == &idle);
}

static void test_partition_mismatch_fails_closed(void)
{
    bh_thread_t candidate;
    bh_thread_t idle;
    init_candidate(&candidate, 0U);
    init_candidate(&idle, 0U);
    g_partition_allows = false;

    assert(sched_validate_picked_candidate(&candidate, &idle, 0U) == &idle);
}

static void test_policy_is_selected_per_core(void)
{
    bh_thread_t candidate;
    init_candidate(&candidate, 1U);
    candidate.affinity_mask = 1U << 1U;
    candidate.constraints.cpu_mask = 1U << 1U;
    g_partition_allows = true;

    assert(sched_is_core_admissible(&candidate, 1) == true);
    candidate.owner_cpu = 0U;
    candidate.affinity_mask = 1U;
    candidate.constraints.cpu_mask = 1U;
    assert(sched_is_core_admissible(&candidate, 0) == true);
    assert(g_last_class[0] == BHARAT_SCHED_CLASS_FAIR);
    assert(g_last_class[1] == BHARAT_SCHED_CLASS_DEADLINE_RT);
}

static void test_owner_mismatch_fails_closed(void)
{
    bh_thread_t candidate;
    bh_thread_t idle;
    init_candidate(&candidate, 1U);
    init_candidate(&idle, 0U);
    g_partition_allows = true;

    assert(sched_validate_picked_candidate(&candidate, &idle, 0U) == &idle);
}

static void test_out_of_range_core_fails_closed(void)
{
    bh_thread_t candidate;
    bh_thread_t idle;
    init_candidate(&candidate, 32U);
    init_candidate(&idle, 0U);
    g_partition_allows = true;

    /* Affinity and constraint masks are 32-bit, so core 32 is never valid. */
    assert(sched_validate_picked_candidate(&candidate, &idle, 32U) == &idle);
}

static void test_context_switch_is_counted_once(void)
{
    sched_rq_t rq;
    bh_thread_t next;
    memset(&rq, 0, sizeof(rq));
    memset(&next, 0, sizeof(next));

    sched_account_context_switch(&rq, &next);

    assert(rq.context_switches == 1U);
    assert(next.context_switch_count == 1U);
}

static void test_invalid_context_switch_is_not_counted(void)
{
    sched_rq_t rq;
    bh_thread_t next;
    memset(&rq, 0, sizeof(rq));
    memset(&next, 0, sizeof(next));

    sched_account_context_switch(NULL, &next);
    sched_account_context_switch(&rq, NULL);

    assert(rq.context_switches == 0U);
    assert(next.context_switch_count == 0U);
}

static void test_idle_remains_selectable_without_candidate(void)
{
    bh_thread_t idle;
    init_candidate(&idle, 0U);
    idle.flags = BH_THREAD_FLAG_IDLE;

    assert(sched_validate_picked_candidate(NULL, &idle, 0U) == &idle);
}

int main(void)
{
    test_inadmissible_task_is_never_selected();
    test_affinity_mismatch_fails_closed();
    test_partition_mismatch_fails_closed();
    test_policy_is_selected_per_core();
    test_owner_mismatch_fails_closed();
    test_out_of_range_core_fails_closed();
    test_context_switch_is_counted_once();
    test_invalid_context_switch_is_not_counted();
    test_idle_remains_selectable_without_candidate();
    puts("All scheduler selection host tests passed.");
    return 0;
}
