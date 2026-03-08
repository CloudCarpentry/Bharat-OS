#include "advanced/algo_matrix.h"
#include "arch/capabilities.h"
#include "hal/hal.h"
#include <stddef.h>

/*
 * Bharat-OS Algorithm Capability Matrix Implementation
 *
 * Implements a 4-tier selection system for critical algorithms.
 * Resolves optimal capabilities at boot based on available hardware and build features.
 */

sched_algo_ops_t  g_sched_ops;
search_algo_ops_t g_search_ops;

/* Fallbacks to be assigned in their respective modules */
extern struct kthread* sched_pick_next_ready_l0(uint32_t core_id);
extern void sched_enqueue_task_l0(struct kthread* thread, uint32_t core_id);
extern void sched_dequeue_task_l0(struct kthread* thread, uint32_t core_id);

extern int device_lookup_mmio_window_l0(uint32_t class_id, uint32_t device_id, uint32_t window_id, void* out_window);

/* Optimized Level 1 (SMP/per-CPU) implementations */
extern struct kthread* sched_pick_next_ready_l1(uint32_t core_id);
extern void sched_enqueue_task_l1(struct kthread* thread, uint32_t core_id);
extern void sched_dequeue_task_l1(struct kthread* thread, uint32_t core_id);

extern int device_lookup_mmio_window_l1(uint32_t class_id, uint32_t device_id, uint32_t window_id, void* out_window);

/* Accelerator Level 3 Stubs */
// extern int my_accelerator_lookup(uint32_t class_id, ...);

void algo_matrix_init(void) {
    uint32_t num_cores = hal_cpu_get_id() + 1; // Since hal_cpu_get_count might not be available in host tests

    /* 1) Run-Queue Algorithm Resolution */
    if (num_cores > 1) {
        // Multi-threaded SMP logic for Level 1
        g_sched_ops.pick_next_ready = sched_pick_next_ready_l1;
        g_sched_ops.enqueue_task    = sched_enqueue_task_l1;
        g_sched_ops.dequeue_task    = sched_dequeue_task_l1;
    } else {
        // Fallback Reference Logic Level 0
        g_sched_ops.pick_next_ready = sched_pick_next_ready_l0;
        g_sched_ops.enqueue_task    = sched_enqueue_task_l0;
        g_sched_ops.dequeue_task    = sched_dequeue_task_l0;
    }

    /* 2) Search/Sort Operations Resolution */
    if (num_cores > 1) {
        g_search_ops.device_lookup_mmio_window = device_lookup_mmio_window_l1;
    } else {
        g_search_ops.device_lookup_mmio_window = device_lookup_mmio_window_l0;
    }

    /* If we had AVX2/Vector, we would map g_search_ops.my_sort = my_sort_l2 */
    // if (arch_has_feature_avx2() || arch_has_feature_vector()) {
    //      ... Level 2 mapping
    // }

    /* If we had an NPU/TPU ready */
    // if (npu_present) {
    //      ... Level 3 mapping
    // }
}
