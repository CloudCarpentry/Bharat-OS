#include "advanced/ai_kernel_bridge.h"

#include "sched.h"
#include "numa.h"

int ai_kernel_apply_suggestion(const ai_suggestion_t* suggestion) {
    if (!suggestion) {
        return -1;
    }

    if (suggestion->action == AI_ACTION_MIGRATE_TASK) {
        uint32_t active_nodes = numa_active_node_count();
        if (active_nodes == 0U || suggestion->value >= active_nodes) {
            return -1;
        }
    }

    return sched_ai_apply_suggestion(suggestion);
}

int ai_kernel_collect_telemetry(kernel_telemetry_t* out) {
    if (!out) {
        return -1;
    }

    kthread_t* current = sched_current_thread();
    if (!current) {
        return -1;
    }

    uint64_t slice = current->time_slice_ms ? current->time_slice_ms : 1U;
    uint64_t usage = (current->cpu_time_consumed * 100U) / slice;

    out->cpu_usage_pct = (uint32_t)((usage > 100U) ? 100U : usage);
    out->ipc_latency_ns = slice * 1000U;
    out->cache_miss_rate = current->priority * 4U;
    out->context_switches = (uint32_t)(current->cpu_time_consumed / slice);
    out->numa_node_id = (uint8_t)numa_get_current_node();

    return 0;
}
