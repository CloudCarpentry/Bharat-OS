#ifndef BHARAT_AI_SCHED_H
#define BHARAT_AI_SCHED_H

#include <stdint.h>
#include "../sched.h"

/*
 * Bharat-OS AI-Native Resource Governor
 * Tracks Cycles Per Instruction (CPI) and predicts high-complexity tasks
 * (e.g., 5G handover or 4K video streams) to ramp up semiconductor frequency pre-emptively.
 */

typedef struct {
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
