#ifndef BHARAT_KERNEL_SCHED_CPU_PARTITION_VALIDATE_H
#define BHARAT_KERNEL_SCHED_CPU_PARTITION_VALIDATE_H

#include <kernel/status.h>
#include <bharat/uapi/system/execution_mode.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate CPU partitioning invariants.
 *
 * This function performs deep validation of a resolved CPU partition
 * configuration to ensure it meets kernel safety and architectural invariants.
 *
 * @param config The execution configuration to validate.
 * @return K_OK on success, error code otherwise.
 */
kstatus_t cpu_partition_validate(const bharat_execution_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_SCHED_CPU_PARTITION_VALIDATE_H
