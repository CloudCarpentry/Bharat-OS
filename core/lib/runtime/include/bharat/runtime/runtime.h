#ifndef BHARAT_RUNTIME_H
#define BHARAT_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/abi_types.h>
#include <bharat/uapi/bootstrap/service_launch.h>

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
void bharat_runtime_init(const void *startup_ptr);

/**
 * @brief Shutdown the runtime framework gracefully.
 */
void bharat_runtime_shutdown(void);

/**
 * @brief Obtain a bootstrap capability handle provided by the environment.
 * @return The bootstrap capability handle, or BHARAT_INVALID_HANDLE.
 */
bharat_handle_t bharat_runtime_get_bootstrap_cap(void);

/**
 * @brief Simple log hook for service output.
 * @param msg The message string to log.
 */
void bharat_runtime_log(const char *msg);

const struct bharat_user_startup *bharat_runtime_get_startup(void);
int bharat_bootstrap_launch(const char *name, uint32_t service_id,
                           uint32_t namesvc_cap, uint32_t delegate_launch,
                           bh_bootstrap_launch_result_t *out);
int bharat_bootstrap_probe(void);
int bharat_bootstrap_stop(uint32_t process_cap);
int bharat_bootstrap_report(uint32_t type, int32_t status);
int bharat_bootstrap_poll(uint32_t receive_cap, bh_bootstrap_service_event_t *event);
int bharat_runtime_now_ns(uint64_t *out);

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
