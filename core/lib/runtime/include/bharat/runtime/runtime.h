#ifndef BHARAT_RUNTIME_H
#define BHARAT_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>
#include <bharat/ipc/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file runtime.h
 * @brief Minimal native runtime for Bharat services/apps.
 */

/**
 * @brief Initialize the runtime framework for the process.
 */
void bharat_runtime_init(void);

/**
 * @brief Shutdown the runtime framework gracefully.
 */
void bharat_runtime_shutdown(void);

/**
 * @brief Obtain a bootstrap capability handle provided by the environment.
 * @return The bootstrap capability handle, or BHARAT_CAP_INVALID_HANDLE.
 */
bharat_cap_handle_t bharat_runtime_get_bootstrap_cap(void);

/**
 * @brief Simple log hook for service output.
 * @param msg The message string to log.
 */
void bharat_runtime_log(const char *msg);

/**
 * @brief Panic the runtime process.
 * @param reason Reason string.
 */
void bharat_runtime_panic(const char *reason) __attribute__((noreturn));

/**
 * @brief Basic service main loop entry point wrapper.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param main_fn Function pointer to the actual service entry.
 * @return Exit status.
 */
int bharat_runtime_main_wrapper(int argc, char **argv, int (*main_fn)(int, char**));

#ifdef __cplusplus
}
#endif

#endif // BHARAT_RUNTIME_H
