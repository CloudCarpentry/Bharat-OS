#ifndef BHARAT_ALGO_MATRIX_H
#define BHARAT_ALGO_MATRIX_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Algorithm Capability Matrix
 *
 * Implements a 4-tier selection system for critical algorithms:
 *
 * Level 0: Reference C implementation (Works everywhere, fallback).
 * Level 1: Multi-threaded / SMP optimized (Per-core, lock-free).
 * Level 2: ISA Optimized (Intrinsics, Vector instructions, assembly).
 * Level 3: Accelerator Optimized (NPU/TPU specific instructions, function pointer stubs).
 */

typedef enum {
    ALGO_LEVEL_0_REFERENCE = 0,
    ALGO_LEVEL_1_SMP       = 1,
    ALGO_LEVEL_2_ISA       = 2,
    ALGO_LEVEL_3_ACCEL     = 3
} algo_tier_level_t;

/* --- Scheduler Run-Queue Algorithm Ops --- */

// Forward declarations
struct kthread;

typedef struct {
    struct kthread* (*pick_next_ready)(uint32_t core_id);
    void (*enqueue_task)(struct kthread* thread, uint32_t core_id);
    void (*dequeue_task)(struct kthread* thread, uint32_t core_id);
} sched_algo_ops_t;

/* --- Generic Search/Sort Algorithm Ops --- */

typedef struct {
    int (*device_lookup_mmio_window)(uint32_t class_id, uint32_t device_id, uint32_t window_id, void* out_window);
    // Other generic searching/sorting can be added here
} search_algo_ops_t;


/* Global Operation Vectors */
extern sched_algo_ops_t  g_sched_ops;
extern search_algo_ops_t g_search_ops;

/* Capability Resolution */
void algo_matrix_init(void);

#endif /* BHARAT_ALGO_MATRIX_H */
