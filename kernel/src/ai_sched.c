#include "advanced/ai_sched.h"

#include <stddef.h>

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
        ctx->current_cpi = (float)cycles_delta / (float)inst_delta;
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

    float cpi_sum = 0.0f;
    for (uint32_t i = 0; i < 10U; ++i) {
        cpi_sum += ctx->historical_cpi_window[i];
    }

    float cpi_avg = cpi_sum / 10.0f;
    if (cpi_avg > 2.0f) {
        ctx->predicted_complexity = 2U;
    } else if (cpi_avg > 1.0f) {
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
