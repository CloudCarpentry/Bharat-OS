#ifndef BHARAT_KERNEL_SCHED_ADMISSION_H
#define BHARAT_KERNEL_SCHED_ADMISSION_H

#include <kernel/status.h>
#include <bharat/uapi/system/execution_mode.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if a CPU supports a specific scheduler class.
 *
 * @param cpu_id The ID of the CPU.
 * @param class_mask The class mask to check.
 * @return K_OK if allowed, K_ERR_DENIED if not allowed, or other error code.
 */
kstatus_t sched_admission_allows_class(
    uint32_t cpu_id,
    bharat_sched_class_mask_t class_mask
);

/**
 * @brief Select the first eligible CPU for a specific scheduler class.
 *
 * This implementation is deterministic and intentionally simple: it chooses
 * the first eligible CPU in ascending order of CPU ID.
 *
 * @param class_mask The class mask required.
 * @param out_cpu_id Pointer to store the selected CPU ID.
 * @return K_OK on success, K_ERR_NOT_FOUND if no eligible CPU exists.
 */
kstatus_t sched_admission_select_cpu(
    bharat_sched_class_mask_t class_mask,
    uint32_t *out_cpu_id
);

/**
 * @brief Validate the current CPU partition configuration.
 *
 * Reports errors if the configuration violates scheduler invariants:
 * - At least one SYSTEM CPU must exist.
 * - RT and FAIR/GP must not be collapsed on the same CPU in multi-core systems.
 * - Single-core systems must use TEMPORAL strategy.
 *
 * @return K_OK on success, error code otherwise.
 */
kstatus_t sched_admission_validate_partitions(void);

/**
 * @brief Convert execution mode to human-readable string.
 * @param mode The execution mode.
 * @return A stable non-NULL string.
 */
const char *sched_admission_mode_to_string(bharat_execution_mode_t mode);

/**
 * @brief Convert partition strategy to human-readable string.
 * @param strategy The partition strategy.
 * @return A stable non-NULL string.
 */
const char *sched_admission_partition_to_string(bharat_partition_strategy_t strategy);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_SCHED_ADMISSION_H
