#ifndef BHARAT_KERNEL_SCHED_AI_SCHED_H
#define BHARAT_KERNEL_SCHED_AI_SCHED_H

#include <stdint.h>

/*
 * Bounded kernel-facing contract for AI scheduler hints.
 * This is mechanism only; policy resides in staging/ai/ or services.
 */

typedef struct {
    uint32_t cpu_usage_pct;
    uint32_t approx_cpu_util_pct;
    uint64_t ipc_latency_ns;
    uint32_t cache_miss_rate;
    uint32_t context_switches;
    uint64_t cycles;
    uint64_t instructions;
    uint8_t  numa_node_id;
    uint32_t run_queue_depth;
} kernel_telemetry_t;

typedef enum {
    AI_ACTION_NONE = 0,
    AI_ACTION_MIGRATE_TASK,
    AI_ACTION_ADJUST_PRIORITY,
    AI_ACTION_THROTTLE_CORE,
    AI_ACTION_KILL_TASK
} ai_action_t;

typedef struct {
    ai_action_t action;
    uint32_t    target_id;
    uint32_t    value;
} ai_suggestion_t;

typedef struct {
    uint8_t available;
    uint64_t cycles_delta;
    uint64_t instructions_delta;
} ai_pmc_sample_t;

typedef struct ai_sched_context {
    uint32_t thread_id;
    uint8_t  priority;
    kernel_telemetry_t metrics;
    void* private_data;

    uint64_t total_cycles;
    uint64_t total_instructions;
    uint32_t current_cpi;
    uint32_t historical_cpi_window[10];
    uint32_t window_index;
    uint32_t predicted_complexity;

    uint32_t model_id;
    uint32_t flags;
    uint64_t last_suggestion_id;
} ai_sched_context_t;

static inline void ai_sched_init_context(ai_sched_context_t *ctx) {
    if (ctx) {
        ctx->thread_id = 0;
        ctx->model_id = 0;
        ctx->flags = 0;
        ctx->last_suggestion_id = 0;
    }
}

static inline void ai_sched_calibrate_silicon(void) {}

static inline void ai_sched_collect_sample(ai_sched_context_t *ctx,
                             uint64_t time_slice_ms,
                             uint64_t cpu_time_consumed,
                             uint32_t run_queue_depth,
                             uint32_t context_switches) {
    (void)ctx; (void)time_slice_ms; (void)cpu_time_consumed;
    (void)run_queue_depth; (void)context_switches;
}

#endif
