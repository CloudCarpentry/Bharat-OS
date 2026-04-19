#include "runtime_host.h"

/*
 * Skeleton implementation of the Runtime Hosting Layer.
 *
 * This layer will eventually translate these stable ABI calls into
 * underlying Bharat-OS native capabilities and URPC messages.
 * Personalities (Linux, Android) and directly-hosted runtimes (Java, Python)
 * link against this library.
 */

int bharat_runtime_init(void) {
    // TODO: Initialize capability arena and establish URPC connection to service manager.
    return 0;
}

int bharat_runtime_spawn(bharat_runtime_handle_t executable_handle, const char* const args[], bharat_runtime_handle_t* out_process_handle) {
    if (!out_process_handle) return -1;
    // TODO: Construct SpawnRequest over URPC to lifecycle manager.
    *out_process_handle = BHARAT_RUNTIME_INVALID_HANDLE;
    return -1; // Unimplemented
}

void bharat_runtime_report_state(bharat_runtime_state_t state) {
    // TODO: Send state update over URPC to lifecycle manager.
    (void)state;
}

int bharat_runtime_thread_create(bharat_runtime_handle_t* out_thread_handle, bharat_thread_func_t func, void* arg) {
    if (!out_thread_handle || !func) return -1;
    // TODO: Map to underlying kernel thread creation capability.
    *out_thread_handle = BHARAT_RUNTIME_INVALID_HANDLE;
    return -1; // Unimplemented
}

int bharat_runtime_thread_join(bharat_runtime_handle_t thread_handle, void** out_result) {
    // TODO: Wait on thread capability handle.
    (void)thread_handle;
    if (out_result) *out_result = NULL;
    return -1; // Unimplemented
}

int bharat_runtime_service_lookup(const char* service_name, bharat_runtime_handle_t* out_endpoint_handle) {
    if (!service_name || !out_endpoint_handle) return -1;
    // TODO: Perform URPC lookup against local namespace manager.
    *out_endpoint_handle = BHARAT_RUNTIME_INVALID_HANDLE;
    return -1; // Unimplemented
}

int bharat_runtime_ipc_call(bharat_runtime_handle_t endpoint_handle, const void* req_buf, size_t req_len, void* res_buf, size_t res_len, size_t* out_res_len) {
    // TODO: Perform URPC synchronous send/receive.
    (void)endpoint_handle;
    (void)req_buf;
    (void)req_len;
    (void)res_buf;
    (void)res_len;
    if (out_res_len) *out_res_len = 0;
    return -1; // Unimplemented
}

int bharat_runtime_file_open(const char* path, int flags, bharat_runtime_handle_t* out_file_handle) {
    if (!path || !out_file_handle) return -1;
    // TODO: Resolve path against VFS capability root via URPC.
    *out_file_handle = BHARAT_RUNTIME_INVALID_HANDLE;
    return -1; // Unimplemented
}

int64_t bharat_runtime_read(bharat_runtime_handle_t handle, void* buf, size_t count) {
    // TODO: URPC read request to the capability handle.
    (void)handle;
    (void)buf;
    (void)count;
    return -1; // Unimplemented
}

int64_t bharat_runtime_write(bharat_runtime_handle_t handle, const void* buf, size_t count) {
    // TODO: URPC write request to the capability handle.
    (void)handle;
    (void)buf;
    (void)count;
    return -1; // Unimplemented
}

int bharat_runtime_close(bharat_runtime_handle_t handle) {
    // TODO: Drop capability handle.
    (void)handle;
    return -1; // Unimplemented
}
