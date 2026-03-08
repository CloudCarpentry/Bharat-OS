#include "advanced/ai_sched.h"

#include <stddef.h>

// @cite Integrating Artificial Intelligence into Operating Systems (Korshun et al., 2024)
// @cite Enhancing Operating System Performance with AI (S. S. et al., 2025)
static ai_heuristic_config_t g_ai_cfg = {
    .penalty_threshold = 5000U,
    .weight_ipc_latency = 2U,
    .weight_cache_miss = 10U,
};

int ai_sched_arch_sample_pmc(uint32_t thread_id, ai_pmc_sample_t* out_sample) __attribute__((weak));
int ai_sched_arch_sample_pmc(uint32_t thread_id, ai_pmc_sample_t* out_sample) {
    (void)thread_id;
    if (!out_sample) {
        return -1;
    }

    out_sample->available = 0U;
    out_sample->cycles_delta = 0U;
    out_sample->instructions_delta = 0U;
    return 0;
}

void ai_sched_init_context(ai_sched_context_t* ctx) {
    if (!ctx) {
        return;
    }

    *ctx = (ai_sched_context_t){0};
}

void ai_sched_update_telemetry(ai_sched_context_t* ctx, uint64_t cycles_delta, uint64_t inst_delta) {
    if (!ctx) {
        return;
    }

    ctx->total_cycles += cycles_delta;
    ctx->total_instructions += inst_delta;

    if (inst_delta != 0U) {
        // Use integer arithmetic (CPI * 100) instead of floats for bare-metal portability
        // to avoid soft-float libgcc dependencies.
        ctx->current_cpi = (uint32_t)((cycles_delta * 100U) / inst_delta);
    } else {
        ctx->current_cpi = 0;
    }

    ctx->historical_cpi_window[ctx->window_index % 10U] = ctx->current_cpi;
    ctx->window_index = (ctx->window_index + 1U) % 10U;

    ctx->metrics.cycles = ctx->total_cycles;
    ctx->metrics.instructions = ctx->total_instructions;
}

void ai_sched_predict_and_scale(ai_sched_context_t* ctx) {
    if (!ctx) {
        return;
    }

    uint32_t cpi_sum = 0;
    for (uint32_t i = 0; i < 10U; ++i) {
        cpi_sum += ctx->historical_cpi_window[i];
    }

    uint32_t cpi_avg = cpi_sum / 10U;
    if (cpi_avg > 200U) {
        ctx->predicted_complexity = 2U;
    } else if (cpi_avg > 100U) {
        ctx->predicted_complexity = 1U;
    } else {
        ctx->predicted_complexity = 0U;
    }
}

void ai_sched_collect_sample(ai_sched_context_t* ctx,
                             uint64_t time_slice_ms,
                             uint64_t cpu_time_consumed,
                             uint32_t run_queue_depth,
                             uint32_t context_switches) {
    if (!ctx) {
        return;
    }

    ai_pmc_sample_t sample = {0};
    (void)ai_sched_arch_sample_pmc(ctx->thread_id, &sample);

    uint64_t cycles_delta = 0U;
    uint64_t inst_delta = 0U;

    if (sample.available != 0U && sample.instructions_delta > 0U) {
        cycles_delta = sample.cycles_delta;
        inst_delta = sample.instructions_delta;
    } else {
#if defined(Profile_RTOS)
        cycles_delta = time_slice_ms * 100000U;
#elif defined(Profile_EDGE)
        cycles_delta = time_slice_ms * 500000U;
#else
        cycles_delta = time_slice_ms * 1000000U;
#endif
        inst_delta = cycles_delta / 2U;
        if (inst_delta == 0U) {
            inst_delta = 1U;
        }
    }

    ai_sched_update_telemetry(ctx, cycles_delta, inst_delta);

    ctx->metrics.context_switches = context_switches;
    ctx->metrics.run_queue_depth = run_queue_depth;
    ctx->metrics.approx_cpu_util_pct =
        (uint32_t)((cpu_time_consumed * 100U) / ((time_slice_ms == 0U) ? 1U : time_slice_ms));

    ai_sched_predict_and_scale(ctx);
}

int ai_heuristic_config_load(ai_heuristic_config_t* out_cfg) {
    if (!out_cfg) {
        return -1;
    }

    *out_cfg = g_ai_cfg;
    return 0;
}

int ai_heuristic_config_store(const ai_heuristic_config_t* cfg) {
    if (!cfg) {
        return -1;
    }

    if (cfg->weight_ipc_latency == 0U || cfg->weight_cache_miss == 0U) {
        return -2;
    }

    g_ai_cfg = *cfg;
    return 0;
}

// Fallback mechanism & Learning-based priority queue

ai_telemetry_status_t ai_telemetry = { .is_active = 0 };

uint8_t ai_model_ready(void) {
    // Mock implementation for model readiness
    return 0;
}

// Mock prediction function for demonstration
struct kthread* ai_model_predict_best(struct kthread *run_queue) {
    (void)run_queue;
    return NULL;
}

// Simple fallback scheduler
struct kthread* fallback_scheduler(struct kthread *run_queue) {
    if (run_queue == NULL) return NULL;
    // In a real kthread_t, next would be handled via list_head_t.
    // For the sake of this conceptual implementation, we'll return run_queue.
    return run_queue;
}

// Main AI selection logic
struct kthread* ai_sched_select_task(struct kthread *run_queue) {
    // 1. Check if AI subsystem is active and model is loaded
    if (!ai_telemetry.is_active || !ai_model_ready()) {
        return fallback_scheduler(run_queue); // Immediate fallback
    }

    // 2. Attempt AI-based prediction
    struct kthread *selected = ai_model_predict_best(run_queue);

    // 3. Final safety check: if prediction fails, use fallback
    if (selected == NULL) {
        return fallback_scheduler(run_queue);
    }

    return selected;
}

// Selects task with highest weight and decays others (simple learning)
learned_task_t* select_and_update_queue(learned_task_t **head) {
    if (*head == NULL) return NULL;

    learned_task_t *best = *head;
    learned_task_t *curr = *head;

    // Find task with highest dynamic weight
    while (curr != NULL) {
        if (curr->dynamic_weight > best->dynamic_weight) {
            best = curr;
        }
        curr = curr->next;
    }

    // Learning Update: Reward the selected task, slightly boost others to prevent starvation
    curr = *head;
    while (curr != NULL) {
        if (curr == best) {
            curr->dynamic_weight -= 1.0f; // Penalty for being served (Fairness)
        } else {
            curr->dynamic_weight += 0.1f; // "Learning" boost for waiting
        }
        curr = curr->next;
    }

    return best;
}
