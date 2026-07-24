#ifndef BHARAT_KERNEL_DS_RCU_H
#define BHARAT_KERNEL_DS_RCU_H

#include <stdint.h>
#include <kernel/status.h>

/**
 * @file bh_rcu.h
 * @brief Kernel RCU (Read-Copy-Update) Baseline Contract
 *
 * This header defines the minimal read-mostly synchronization contract for Bharat-OS.
 * The initial implementation is a "baseline stub" suitable for uniprocessor (UP)
 * or non-preemptive kernel paths.
 */

typedef uint64_t bh_rcu_epoch_t;

/**
 * @brief Enter an RCU read-side critical section.
 *
 * Readers are allowed to nest. While in a read-side critical section,
 * objects protected by RCU must not be reclaimed.
 */
void bh_rcu_read_lock(void);

/**
 * @brief Exit an RCU read-side critical section.
 */
void bh_rcu_read_unlock(void);

/**
 * @brief Return the current RCU epoch.
 */
bh_rcu_epoch_t bh_rcu_current_epoch(void);

/**
 * @brief Block until all current RCU readers have finished their critical sections.
 *
 * For the baseline stub, this may return immediately if the kernel is non-preemptive.
 */
void bh_rcu_synchronize(void);

/**
 * @brief Callback signature for deferred RCU reclamation.
 */
typedef void (*bh_rcu_callback_t)(void *arg);

/**
 * @brief Queue a callback for invocation after a grace period.
 *
 * @param callback Function to invoke.
 * @param arg Argument to pass to the callback.
 * @return K_OK on success, or error code.
 */
kstatus_t bh_rcu_call(bh_rcu_callback_t callback, void *arg);

#endif // BHARAT_KERNEL_DS_RCU_H
