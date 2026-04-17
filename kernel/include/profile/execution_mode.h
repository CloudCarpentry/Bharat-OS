#ifndef BHARAT_KERNEL_PROFILE_EXECUTION_MODE_H
#define BHARAT_KERNEL_PROFILE_EXECUTION_MODE_H

#include <uapi/system/execution_mode.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Execution mode determines compute/scheduling organization:
 * realtime, general_purpose, mixed_critical
 */

/**
 * @brief Initialize the global execution configuration based on CMake defaults and discovery.
 * @return 0 on success, error code otherwise.
 */
int bharat_execution_mode_init(void);

/**
 * @brief Get a pointer to the active execution configuration descriptor.
 * @return Pointer to active config or NULL if not initialized.
 */
const bharat_execution_config_t* bharat_execution_mode_get_config(void);

/**
 * @brief Dump the execution mode boot summary to the console.
 */
void bharat_execution_mode_print_summary(void);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_PROFILE_EXECUTION_MODE_H
