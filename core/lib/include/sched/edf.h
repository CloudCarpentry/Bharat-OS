#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @file edf.h
 * @brief Earliest Deadline First (EDF) structures for multikernel real-time scheduling.
 *
 * Designed to provide optimal real-time constraints for tasks under a specific core.
 * Relies heavily on accurate, low-overhead hardware timer interrupts (x86 `HPET`,
 * ARM `Generic Timer`, RISC-V `CLINT`) for scheduling adjustments.
 */

#define MAX_RT_TASKS 256

/**
 * @struct rt_task_t
 * @brief The real-time task control block structure containing constraints.
 */
typedef struct {
    uint64_t deadline;      /**< Absolute deadline (e.g., in cycles or ns) */
    uint64_t period;        /**< Period for periodic tasks */
    uint32_t core_affinity; /**< Targeted CPU core */
    void *tcb;              /**< Opaque pointer to the thread control block */
} rt_task_t;

/**
 * @struct rt_runqueue_t
 * @brief Per-core EDF runqueue.
 *
 * Designed to be protected by an MCS lock or lock-free linked list for updates,
 * avoiding global sched locks.
 */
typedef struct {
    rt_task_t *tasks[MAX_RT_TASKS]; /**< Priority queue elements */
    uint32_t count;                 /**< Number of active RT tasks */
    void *lock;                     /**< Opaque pointer to an MCS lock or spinlock */
} rt_runqueue_t;

/**
 * @brief Initialize an EDF runqueue.
 *
 * @param rq The runqueue to initialize.
 * @param lock Optional pointer to an external lock structure (like mcs_lock_t).
 */
void edf_rq_init(rt_runqueue_t *rq, void *lock);

/**
 * @brief Enqueue a real-time task into the EDF runqueue.
 *
 * @param rq The runqueue to enqueue into.
 * @param task The real-time task to add.
 * @return 0 on success, or an error if the queue is full.
 */
int edf_enqueue(rt_runqueue_t *rq, rt_task_t *task);

/**
 * @brief Dequeue a real-time task from the EDF runqueue.
 *
 * @param rq The runqueue to dequeue from.
 * @param task The specific task to remove.
 * @return 0 on success, or an error if the task is not found.
 */
int edf_dequeue(rt_runqueue_t *rq, rt_task_t *task);

/**
 * @brief Select the next highest-priority real-time task based on deadline.
 *
 * @param rq The runqueue to evaluate.
 * @param now The current hardware timestamp.
 * @return The selected task, or NULL if none are ready.
 */
rt_task_t *edf_schedule(rt_runqueue_t *rq, uint64_t now);
