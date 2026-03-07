#include "advanced/ai_kernel_bridge.h"

#include "sched.h"
#include "ipc_endpoint.h"

uint32_t numa_active_node_count(void) __attribute__((weak));
uint32_t numa_get_current_node(void) __attribute__((weak));

static uint32_t ai_bridge_active_node_count(void) {
    if (numa_active_node_count) {
        return numa_active_node_count();
    }
    return 1U;
}

static uint32_t ai_bridge_current_node(void) {
    if (numa_get_current_node) {
        return numa_get_current_node();
    }
    return 0U;
}

int ai_kernel_create_governor_endpoint(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    return ipc_endpoint_create(table, out_send_cap, out_recv_cap);
}

int ai_kernel_ingest_suggestion_ipc(capability_table_t* table, uint32_t recv_cap) {
    if (!table) {
        return -1;
    }

    ai_suggestion_t suggestion = {0};
    uint32_t received_len = 0U;
    int st = ipc_endpoint_receive(table, recv_cap, &suggestion, sizeof(suggestion), &received_len);
    if (st != IPC_OK) {
        return st;
    }

    if (received_len < sizeof(ai_suggestion_t)) {
        return -2;
    }

    if (suggestion.action <= AI_ACTION_NONE || suggestion.action > AI_ACTION_THROTTLE_CORE) {
        return -3;
    }

    return sched_enqueue_ai_suggestion(&suggestion);
}

int ai_kernel_apply_suggestion(const ai_suggestion_t* suggestion) {
    if (!suggestion) {
        return -1;
    }

    if (suggestion->action == AI_ACTION_MIGRATE_TASK) {
        uint32_t active_nodes = ai_bridge_active_node_count();
        if (active_nodes == 0U || suggestion->value >= active_nodes) {
            return -1;
        }
    }

    return sched_enqueue_ai_suggestion(suggestion);
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
    out->approx_cpu_util_pct = out->cpu_usage_pct;
    out->ipc_latency_ns = slice * 1000U;
    out->cache_miss_rate = current->priority * 4U;
    out->context_switches = (uint32_t)current->context_switch_count;
    out->numa_node_id = (uint8_t)ai_bridge_current_node();
    out->cycles = current->ai_sched_ctx ? current->ai_sched_ctx->metrics.cycles : 0U;
    out->instructions = current->ai_sched_ctx ? current->ai_sched_ctx->metrics.instructions : 0U;
    out->run_queue_depth = 0U;

    return 0;
}
