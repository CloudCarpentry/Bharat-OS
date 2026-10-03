#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define BHARAT_KERNEL_SCHED_AI_SCHED_H
#define hal_timer_monotonic_ticks mock_hal_timer_monotonic_ticks
uint64_t mock_hal_timer_monotonic_ticks(void) { return 1000; }

struct bh_thread;
bool sched_is_core_admissible(struct bh_thread *t, int cpu_id) { return true; }

#include "../../../../staging/ai/ai_sched.h"

// Define test variables
#define g_silicon_alu_ipc test_g_silicon_alu_ipc
#define g_silicon_mem_ipc test_g_silicon_mem_ipc

#include "../../../../staging/ai/ai_sched.c"

#undef g_silicon_alu_ipc
#undef g_silicon_mem_ipc
extern uint32_t test_g_silicon_alu_ipc;
extern uint32_t test_g_silicon_mem_ipc;

void test_fallback_uncalibrated() {
    test_g_silicon_alu_ipc = 0;
    test_g_silicon_mem_ipc = 0;

    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);
    ctx.predicted_complexity = 0;

    ai_sched_collect_sample(&ctx, 10, 10, 0, 0);
    assert(ctx.metrics.instructions == 5000000);
}

void test_partial_calibrated_alu() {
    test_g_silicon_alu_ipc = 200;
    test_g_silicon_mem_ipc = 0;

    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);
    ctx.predicted_complexity = 0; // ALU Heavy

    ai_sched_collect_sample(&ctx, 10, 10, 0, 0);
    assert(ctx.metrics.instructions == 20000000);

    ctx.predicted_complexity = 2; // Mem Heavy
    ai_sched_collect_sample(&ctx, 10, 10, 0, 0);
    assert(ctx.metrics.instructions == 25000000);
}

void test_partial_calibrated_mem() {
    test_g_silicon_alu_ipc = 0;
    test_g_silicon_mem_ipc = 100;

    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);

    ctx.predicted_complexity = 1; // Medium = (50 + 100) / 2 = 75
    ai_sched_collect_sample(&ctx, 10, 10, 0, 0);
    assert(ctx.metrics.instructions == 7500000);
}

void test_both_calibrated() {
    test_g_silicon_alu_ipc = 150;
    test_g_silicon_mem_ipc = 100;

    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);

    ctx.predicted_complexity = 1; // Medium = (150 + 100) / 2 = 125
    ai_sched_collect_sample(&ctx, 10, 10, 0, 0);
    assert(ctx.metrics.instructions == 12500000);
}

void test_zero_time_slice() {
    test_g_silicon_alu_ipc = 100;

    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);

    ai_sched_collect_sample(&ctx, 0, 10, 0, 0);
    assert(ctx.metrics.instructions == 0);
}

void test_zero_ticks_calibration() {
    uint32_t val = calculate_baseline_ipc(0);
    assert(val == 50); // Fallback instead of 200!
}

void test_saturation_wrapper() {
    test_g_silicon_alu_ipc = 200;
    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);
    ctx.predicted_complexity = 0; // ALU Heavy

    // Pass fake sample by mocking ai_sched_arch_sample_pmc inside local scope or test
    // Let's pass huge time_slice_ms
    uint64_t huge_slice = 0xFFFFFFFFFFFFFFFFULL / 1000000ULL;
    ai_sched_collect_sample(&ctx, huge_slice, 10, 0, 0);

    // We expect inst_delta to be UINT64_MAX due to saturation!
    assert(ctx.metrics.instructions == UINT64_MAX);
}

void test_cumulative_saturation() {
    test_g_silicon_alu_ipc = 100;
    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);
    ctx.predicted_complexity = 0;

    ctx.total_instructions = 0xFFFFFFFFFFFFFFFEULL;
    ai_sched_collect_sample(&ctx, 10, 10, 0, 0);
    assert(ctx.metrics.instructions == UINT64_MAX);
}

void test_no_fabricated_instructions() {
    test_g_silicon_alu_ipc = 100;
    ai_sched_context_t ctx;
    ai_sched_init_context(&ctx);
    ctx.predicted_complexity = 0; // ALU Heavy

    // Cycles = 0
    ai_sched_collect_sample(&ctx, 0, 0, 0, 0);
    assert(ctx.metrics.instructions == 0);
    assert(ctx.current_cpi == 0);
}


int main() {
    test_fallback_uncalibrated();
    test_partial_calibrated_alu();
    test_partial_calibrated_mem();
    test_both_calibrated();
    test_zero_time_slice();
    test_zero_ticks_calibration();
    test_saturation_wrapper();
    test_cumulative_saturation();
    test_no_fabricated_instructions();
    printf("All focused tests passed!\n");
    return 0;
}
