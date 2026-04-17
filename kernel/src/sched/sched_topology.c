#include <sched/sched_topology.h>
#include <sched/cpu_partition.h>
#include <profile/execution_mode.h>
#include <stddef.h>

/* Forward declare what we need from sched_internal.h to scaffold binding */
struct sched_runqueue {
    uint32_t cpu_id;
    bharat_cpu_partition_role_t partition_role;
    uint32_t allowed_classes;
    bool allow_migration_in;
    bool allow_migration_out;
};

// Assuming global/array of runqueues exists somewhere in the scheduler
// We'll mock a simple registry here to prove the binding works.
#define MAX_RUNQUEUES BHARAT_MAX_CPU_PARTITIONS
static struct sched_runqueue g_mock_rqs[MAX_RUNQUEUES] = {0};

int sched_topology_bind_cpu(uint32_t cpu_id, const bharat_cpu_partition_desc_t *desc) {
    if (!desc || cpu_id >= MAX_RUNQUEUES) {
        return -1;
    }

    struct sched_runqueue *rq = &g_mock_rqs[cpu_id];

    rq->cpu_id = cpu_id;
    rq->partition_role = desc->role;
    rq->allowed_classes = desc->allowed_sched_classes;
    rq->allow_migration_in = desc->allow_migration_in;
    rq->allow_migration_out = desc->allow_migration_out;

    return 0;
}

int sched_topology_validate_rq(struct sched_runqueue *rq) {
    if (!rq) return -1;

    // Check against config
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(rq->cpu_id);
    if (!desc) return -2;

    if (rq->partition_role != desc->role) return -3;
    if (rq->allowed_classes != desc->allowed_sched_classes) return -4;

    return 0;
}
