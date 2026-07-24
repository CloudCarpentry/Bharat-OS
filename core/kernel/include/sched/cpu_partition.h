#ifndef BHARAT_KERNEL_SCHED_CPU_PARTITION_H
#define BHARAT_KERNEL_SCHED_CPU_PARTITION_H

#include <bharat/uapi/system/execution_mode.h>
#include <kernel/status.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize CPU partitioning based on execution config.
 *
 * Validates the CPU configuration and resolves the partitioning strategy
 * (temporal vs spatial) based on the number of active CPUs and the
 * requested execution mode.
 *
 * @param config The execution configuration to validate and update.
 * @return K_OK on success, error code otherwise.
 */
kstatus_t cpu_partition_init(bharat_execution_config_t *config);

/**
 * @brief Get the partition descriptor for a specific CPU.
 * @param cpu_id The ID of the CPU.
 * @return Pointer to the descriptor or NULL if invalid.
 */
const bharat_cpu_partition_desc_t* cpu_partition_get(uint32_t cpu_id);

/**
 * @brief Check if a CPU supports a specific scheduler class.
 *
 * This is currently an alias for any-match behavior.
 *
 * @param cpu_id The ID of the CPU.
 * @param class_mask The class mask to check.
 * @return true if allowed, false otherwise.
 */
bool cpu_partition_allows_class(uint32_t cpu_id, bharat_sched_class_mask_t class_mask);

/**
 * @brief Check if a CPU supports ANY of the specified scheduler classes.
 * @param cpu_id The ID of the CPU.
 * @param class_mask The class mask to check.
 * @return true if ANY class is allowed, false otherwise.
 */
bool cpu_partition_allows_any_class(uint32_t cpu_id, bharat_sched_class_mask_t class_mask);

/**
 * @brief Check if a CPU supports ALL of the specified scheduler classes.
 * @param cpu_id The ID of the CPU.
 * @param class_mask The class mask to check.
 * @return true if ALL classes are allowed, false otherwise.
 */
bool cpu_partition_allows_all_classes(uint32_t cpu_id, bharat_sched_class_mask_t class_mask);

/**
 * @brief Check if a CPU is designated for housekeeping tasks.
 * @param cpu_id The ID of the CPU.
 * @return true if it is a housekeeping CPU, false otherwise.
 */
bool cpu_partition_is_housekeeping(uint32_t cpu_id);

/**
 * @brief Check if a CPU is dedicated to Real-Time tasks.
 * @param cpu_id The ID of the CPU.
 * @return true if it is an RT-dedicated CPU, false otherwise.
 */
bool cpu_partition_is_rt_dedicated(uint32_t cpu_id);

/**
 * @brief Check if a CPU can accept migration of tasks of a specific scheduler class.
 * @param cpu_id The ID of the CPU.
 * @param class_mask The class mask of the task.
 * @return true if migration-in is allowed, false otherwise.
 */
bool cpu_partition_can_accept_migration(uint32_t cpu_id, bharat_sched_class_mask_t class_mask);

/**
 * @brief Check if a CPU can allow tasks of a specific scheduler class to migrate out.
 * @param cpu_id The ID of the CPU.
 * @param class_mask The class mask of the task.
 * @return true if migration-out is allowed, false otherwise.
 */
bool cpu_partition_can_send_migration(uint32_t cpu_id, bharat_sched_class_mask_t class_mask);

/**
 * @brief Check if a CPU should receive interrupts.
 * @param cpu_id The ID of the CPU.
 * @return true if it is a preferred IRQ target, false otherwise.
 */
bool cpu_partition_should_receive_irq(uint32_t cpu_id);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_SCHED_CPU_PARTITION_H
