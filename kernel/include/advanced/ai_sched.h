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

typedef struct ai_telemetry_status {
    uint8_t is_active;
} ai_telemetry_status_t;

extern ai_telemetry_status_t ai_telemetry;

// Mock AI functions
uint8_t ai_model_ready(void);

// Calibration
void ai_sched_calibrate_silicon(void);

// Task structure definitions for AI priority queue
struct kthread;

typedef struct learned_task {
    uint64_t id;
    int32_t base_weight;
    uint64_t vruntime;
    struct kthread *kthread_ptr;
} learned_task_t;

#ifndef BHARAT_MAX_TASKS
#if defined(Profile_DATACENTER) || defined(Profile_DESKTOP)
#define BHARAT_MAX_TASKS 65536
#elif defined(Profile_EDGE)
#define BHARAT_MAX_TASKS 4096
#else
#define BHARAT_MAX_TASKS 1024
#endif
#endif

typedef struct {
    learned_task_t* nodes[BHARAT_MAX_TASKS];
    uint32_t size;
} learned_task_heap_t;

struct kthread* fallback_scheduler(struct kthread *run_queue);
struct kthread* ai_sched_select_task(struct kthread *run_queue);
void heap_insert(learned_task_heap_t *heap, learned_task_t *task);
learned_task_t* heap_extract_min(learned_task_heap_t *heap);
learned_task_t* select_and_update_queue(learned_task_heap_t *heap, learned_task_t *prev_task, uint64_t execution_time, uint64_t system_weight);

#endif // BHARAT_AI_SCHED_H
