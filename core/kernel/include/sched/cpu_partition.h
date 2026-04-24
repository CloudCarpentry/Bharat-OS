#ifndef BHARAT_KERNEL_SCHED_CPU_PARTITION_H
#define BHARAT_KERNEL_SCHED_CPU_PARTITION_H

#include <bharat/uapi/system/execution_mode.h>

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
 * @return 0 on success, error code otherwise.
 */
int cpu_partition_init(bharat_execution_config_t *config);

/**
 * @brief Get the partition descriptor for a specific CPU.
 * @param cpu_id The ID of the CPU.
 * @return Pointer to the descriptor or NULL if invalid.
 */
const bharat_cpu_partition_desc_t* cpu_partition_get(uint32_t cpu_id);

/**
 * @brief Check if a CPU supports a specific scheduler class.
 * @param cpu_id The ID of the CPU.
 * @param class_mask The class mask to check.
 * @return true if allowed, false otherwise.
 */
bool cpu_partition_allows_class(uint32_t cpu_id, bharat_sched_class_mask_t class_mask);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_SCHED_CPU_PARTITION_H
