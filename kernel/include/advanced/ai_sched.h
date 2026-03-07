#ifndef BHARAT_AI_SCHED_H
#define BHARAT_AI_SCHED_H

#include <stdint.h>

/*
 * Bharat-OS AI-Native Resource Governor
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
    AI_MSG_TYPE_SUGGESTION = 1U,
    AI_MSG_TYPE_TELEMETRY = 2U,
} ai_msg_type_t;

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
    uint64_t penalty_threshold;
    uint32_t weight_ipc_latency;
    uint32_t weight_cache_miss;
} ai_heuristic_config_t;

typedef struct {
    uint8_t available;
    uint64_t cycles_delta;
    uint64_t instructions_delta;
} ai_pmc_sample_t;

typedef struct {
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
} ai_sched_context_t;

void ai_sched_init_context(ai_sched_context_t* ctx);
void ai_sched_update_telemetry(ai_sched_context_t* ctx, uint64_t cycles_delta, uint64_t inst_delta);
void ai_sched_predict_and_scale(ai_sched_context_t* ctx);

/*
 * Pluggable architecture/profile hook entrypoints.
 * Architectures may override ai_sched_arch_sample_pmc in arch HAL code.
 */
int ai_sched_arch_sample_pmc(uint32_t thread_id, ai_pmc_sample_t* out_sample);
void ai_sched_collect_sample(ai_sched_context_t* ctx,
                             uint64_t time_slice_ms,
                             uint64_t cpu_time_consumed,
                             uint32_t run_queue_depth,
                             uint32_t context_switches);

int ai_heuristic_config_load(ai_heuristic_config_t* out_cfg);
int ai_heuristic_config_store(const ai_heuristic_config_t* cfg);

#endif // BHARAT_AI_SCHED_H
