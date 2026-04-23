#ifndef BHARAT_UAPI_RUNTIME_HOST_H
#define BHARAT_UAPI_RUNTIME_HOST_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file runtime_host.h
 * @brief Stable Application Binary Interface (ABI) for the Runtime Hosting Layer.
 *
 * This contract provides a unified, capability-driven abstraction for
 * complex managed runtimes (Java, Python, .NET, Node.js). Personalities
 * (e.g., Linux Compat, Android Compat) and direct native execution environments
 * must rely on this abstraction instead of interacting with kernel primitives directly.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * Opaque Handles & Capabilities
 * ===================================================================== */

/**
 * @brief Opaque handle representing a capability or resource managed by the runtime.
 */
typedef uint64_t bharat_runtime_handle_t;

#define BHARAT_RUNTIME_INVALID_HANDLE ((bharat_runtime_handle_t)0)

/* =====================================================================
 * Process & Lifecycle Hooks
 * ===================================================================== */

typedef enum {
    RUNTIME_STATE_INIT,
    RUNTIME_STATE_RUNNING,
    RUNTIME_STATE_PAUSED,
    RUNTIME_STATE_STOPPING
} bharat_runtime_state_t;

/**
 * @brief Initialize the runtime hosting environment.
 * @return 0 on success, negative error code on failure.
 */
int bharat_runtime_init(void);

/**
 * @brief Spawn a new child process/task bounded by a capability.
 * @param executable_handle Handle to the executable package/binary.
 * @param args Null-terminated array of arguments.
 * @param out_process_handle Pointer to store the spawned process handle.
 * @return 0 on success.
 */
int bharat_runtime_spawn(bharat_runtime_handle_t executable_handle, const char* const args[], bharat_runtime_handle_t* out_process_handle);

/**
 * @brief Report current runtime state to the OS lifecycle manager.
 */
void bharat_runtime_report_state(bharat_runtime_state_t state);

/* =====================================================================
 * Threading & Synchronization Hooks
 * ===================================================================== */

typedef void* (*bharat_thread_func_t)(void* arg);

/**
 * @brief Create a native thread managed by the runtime host.
 * @param out_thread_handle Pointer to store the created thread handle.
 * @param func Entry point function.
 * @param arg Argument passed to the entry point.
 * @return 0 on success.
 */
int bharat_runtime_thread_create(bharat_runtime_handle_t* out_thread_handle, bharat_thread_func_t func, void* arg);

/**
 * @brief Wait for a thread to exit.
 * @param thread_handle Handle of the thread to join.
 * @param out_result Pointer to store the thread's return value.
 * @return 0 on success.
 */
int bharat_runtime_thread_join(bharat_runtime_handle_t thread_handle, void** out_result);

/* =====================================================================
 * IPC & Service Access
 * ===================================================================== */

/**
 * @brief Look up a service endpoint by name in the local namespace.
 * @param service_name Name of the requested service.
 * @param out_endpoint_handle Pointer to store the capability handle for the endpoint.
 * @return 0 on success.
 */
int bharat_runtime_service_lookup(const char* service_name, bharat_runtime_handle_t* out_endpoint_handle);

/**
 * @brief Send a synchronous message to a service endpoint.
 * @param endpoint_handle Handle of the target service endpoint.
 * @param req_buf Request buffer.
 * @param req_len Request buffer length.
 * @param res_buf Response buffer.
 * @param res_len Max response buffer length.
 * @param out_res_len Actual length of the response.
 * @return 0 on success.
 */
int bharat_runtime_ipc_call(bharat_runtime_handle_t endpoint_handle, const void* req_buf, size_t req_len, void* res_buf, size_t res_len, size_t* out_res_len);

/* =====================================================================
 * File & Network Abstraction (VFS/Net URPC Proxies)
 * ===================================================================== */

/**
 * @brief Open a capability-bounded file/resource.
 * @param path Abstract path within the permitted namespace.
 * @param flags Open flags.
 * @param out_file_handle Pointer to store the opened file handle.
 * @return 0 on success.
 */
int bharat_runtime_file_open(const char* path, int flags, bharat_runtime_handle_t* out_file_handle);

/**
 * @brief Read data from a runtime handle (file/socket).
 * @return Number of bytes read, or negative on error.
 */
int64_t bharat_runtime_read(bharat_runtime_handle_t handle, void* buf, size_t count);

/**
 * @brief Write data to a runtime handle (file/socket).
 * @return Number of bytes written, or negative on error.
 */
int64_t bharat_runtime_write(bharat_runtime_handle_t handle, const void* buf, size_t count);

/**
 * @brief Close a runtime handle.
 */
int bharat_runtime_close(bharat_runtime_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_RUNTIME_HOST_H */
