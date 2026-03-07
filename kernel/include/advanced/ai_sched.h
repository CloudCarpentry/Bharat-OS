#ifndef BHARAT_AI_SCHED_H
#define BHARAT_AI_SCHED_H

#include <stdint.h>

/*
 * Bharat-OS AI-Native Resource Governor
 * Tracks Cycles Per Instruction (CPI) and predicts high-complexity tasks
 * (e.g., 5G handover or 4K video streams) to ramp up semiconductor frequency pre-emptively.
 */

/**
 * @brief Real-time telemetry data provided to the AI Governor.
 * This struct forms the input vector for scheduling heuristics.
 */
typedef struct {
    uint32_t cpu_usage_pct;     /* CPU utilization (0-100) */
    uint64_t ipc_latency_ns;    /* Average IPC latency in nanoseconds */
    uint32_t cache_miss_rate;   /* Cache misses per 1k instructions */
    uint32_t context_switches;  /* Frequency of thread swaps */
    uint8_t  numa_node_id;      /* Originating NUMA node */
} kernel_telemetry_t;

typedef enum {
    AI_MSG_TYPE_SUGGESTION = 1U,
    AI_MSG_TYPE_TELEMETRY = 2U,
} ai_msg_type_t;

/**
 * @brief Actions suggested by the AI Governor to the Kernel.
 */
typedef enum {
    AI_ACTION_NONE = 0,
    AI_ACTION_MIGRATE_TASK,     /* Move task to different core/node */
    AI_ACTION_ADJUST_PRIORITY,  /* Dynamic priority scaling */
    AI_ACTION_THROTTLE_CORE,    /* Power management for thermal efficiency */
    AI_ACTION_KILL_TASK         /* Suspend or destroy memory-hungry task */
} ai_action_t;

typedef struct {
    ai_action_t action;
    uint32_t    target_id;      /* Thread ID or Core ID */
    uint32_t    value;          /* New priority level or target Node ID */
} ai_suggestion_t;

// Struct to make heuristic weights configurable
typedef struct {
    uint64_t penalty_threshold;
    uint32_t weight_ipc_latency;
    uint32_t weight_cache_miss;
} ai_heuristic_config_t;

typedef struct {
    uint32_t thread_id;
    uint8_t  priority;
    kernel_telemetry_t metrics; /* Nested telemetry for this context */
    void* private_data;

    uint64_t total_cycles;
    uint64_t total_instructions;
    float current_cpi;

    // Historical model context for lightweight inference
    float historical_cpi_window[10];
    uint32_t window_index;

    // Prediction output
    uint32_t predicted_complexity; // 0 = Low, 1 = Normal, 2 = High (Burst)
} ai_sched_context_t;

// Initialize the AI scheduler context for a given process/thread
void ai_sched_init_context(ai_sched_context_t* ctx);

// Update CPI telemetry from CPU Performance Monitoring Counters (PMCs)
void ai_sched_update_telemetry(ai_sched_context_t* ctx, uint64_t cycles_delta, uint64_t inst_delta);

// Run lightweight inference to determine if pre-emptive frequency scaling is needed
void ai_sched_predict_and_scale(ai_sched_context_t* ctx);

#endif // BHARAT_AI_SCHED_H
