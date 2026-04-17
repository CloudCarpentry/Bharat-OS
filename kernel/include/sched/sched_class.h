#ifndef BHARAT_KERNEL_SCHED_CLASS_H
#define BHARAT_KERNEL_SCHED_CLASS_H

#include <stdint.h>
#include <stdbool.h>
#include <uapi/system/execution_mode.h>

#ifdef __cplusplus
extern "C" {
#endif

struct thread_context; // Forward declaration
struct sched_runqueue; // Forward declaration

/**
 * @brief Scheduler class operations table.
 *
 * Each scheduler class (SYSTEM, FIFO_RT, FAIR, etc.) registers an instance
 * of this structure.
 */
typedef struct sched_class_ops {
    const char *name;
    bharat_sched_class_mask_t class_mask;

    // Hooks (simplified for the framework scaffolding)
    void (*enqueue)(struct sched_runqueue *rq, struct thread_context *thread);
    void (*dequeue)(struct sched_runqueue *rq, struct thread_context *thread);
    struct thread_context* (*pick_next)(struct sched_runqueue *rq);
    void (*tick)(struct sched_runqueue *rq);
} sched_class_ops_t;

/**
 * @brief Register a new scheduler class.
 * @param ops The operations table for the class.
 * @return 0 on success.
 */
int sched_class_register(sched_class_ops_t *ops);

/**
 * @brief Find a registered scheduler class by its mask.
 * @param mask The class mask to look for.
 * @return Pointer to the class ops or NULL if not found.
 */
sched_class_ops_t* sched_class_find_by_mask(bharat_sched_class_mask_t mask);

/**
 * @brief Initialize the scheduler class registry.
 */
void sched_class_registry_init(void);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_SCHED_CLASS_H
