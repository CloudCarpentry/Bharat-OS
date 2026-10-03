#include "runtime_host.h"
#include "../../fs/fs_client.h" // Include fs_client to use fs_open
#include <bharat/libc/alloc.h>
#include <bharat/runtime/runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/ipc/ipc.h>
#include <bharat/uapi/process_manager/contract_v1.h>
#include <bharat/uapi/init/bootstrap.h>
#include <string.h>

#define CAPABILITY_ARENA_SIZE 4096

static uint8_t g_capability_arena_buffer[CAPABILITY_ARENA_SIZE];
static bh_fixed_arena_t g_capability_arena;

static bharat_ipc_endpoint_t g_pm_endpoint = BHARAT_INVALID_HANDLE;

/*
 * Skeleton implementation of the Runtime Hosting Layer.
 *
 * This layer will eventually translate these stable ABI calls into
 * underlying Bharat-OS native capabilities and URPC messages.
 * Personalities (Linux, Android) and directly-hosted runtimes (Java, Python)
 * link against this library.
 */

int bharat_runtime_host_init(const void *startup_ptr) {
    if (!startup_ptr) {
        return -1;
    }

    // Initialize capability arena
    bh_fixed_arena_init(&g_capability_arena, g_capability_arena_buffer, CAPABILITY_ARENA_SIZE);

    // Establish connection to service manager / lifecycle manager (process_manager)
    // The namesvc will use the startup's bootstrap capability implicitly via namesvc_lookup
    // since namesvc_client implementation uses bharat_runtime_get_startup() behind the scenes.
    uint32_t svc_id = 0;
    uint32_t version = 0;
    if (namesvc_lookup("process_manager", &svc_id, &g_pm_endpoint, &version) != BHARAT_STATUS_OK) {
        return -1;
    }

    if (g_pm_endpoint == BHARAT_INVALID_HANDLE) {
        return -1;
    }

    return 0;
}

int bharat_runtime_spawn(bharat_runtime_handle_t executable_handle, const char* const args[], bharat_runtime_handle_t* out_process_handle) {
    if (!out_process_handle) return -1;

    if (g_pm_endpoint == BHARAT_INVALID_HANDLE) {
        *out_process_handle = BHARAT_RUNTIME_INVALID_HANDLE;
        return -1;
    }

    // Construct SpawnRequest to lifecycle manager.
    bh_pm_spawn_request_v1_t spawn_req;
    memset(&spawn_req, 0, sizeof(spawn_req));
    spawn_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    spawn_req.struct_size = sizeof(bh_pm_spawn_request_v1_t);
    spawn_req.executable_handle = (uint64_t)executable_handle;

    if (args && args[0]) {
        // Copy the first argument as process name for simplicity, as per ABI limits
        strncpy(spawn_req.process_name, args[0], sizeof(spawn_req.process_name) - 1);
    }

    bh_pm_spawn_response_v1_t spawn_res;
    memset(&spawn_res, 0, sizeof(spawn_res));

    bharat_ipc_msg_header_t req_hdr;
    memset(&req_hdr, 0, sizeof(req_hdr));
    req_hdr.message_id = BH_PM_OP_SPAWN_V1;
    req_hdr.payload_size = sizeof(spawn_req);

    bharat_ipc_msg_header_t res_hdr;
    memset(&res_hdr, 0, sizeof(res_hdr));

    int32_t call_status = bharat_ipc_call(g_pm_endpoint, &req_hdr, &spawn_req, &res_hdr, &spawn_res, sizeof(spawn_res));

    if (call_status == BHARAT_IPC_STATUS_OK && spawn_res.status == BHARAT_STATUS_OK) {
        *out_process_handle = (bharat_runtime_handle_t)spawn_res.process_handle;
        return 0;
    }

    *out_process_handle = BHARAT_RUNTIME_INVALID_HANDLE;
    return -1;
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

    *out_file_handle = BHARAT_RUNTIME_INVALID_HANDLE;
    // TODO: Stage 3 integration needed - Perform IPC request (FS_OPEN) to filesystem service.
    // Removed unsafe bypass calling direct VFS fd mechanism with dummy capabilities.
    (void)flags;
    return -1; // Unimplemented
}

int64_t bharat_runtime_read(bharat_runtime_handle_t handle, void* buf, size_t count) {
    if (handle == BHARAT_RUNTIME_INVALID_HANDLE) return -1;
    if (count > 0 && !buf) return -1;

    // TODO: Stage 3 integration needed - Perform IPC request (FS_READ) to the resource capability handle.
    return -1; // Unimplemented
}

int64_t bharat_runtime_write(bharat_runtime_handle_t handle, const void* buf, size_t count) {
    if (handle == BHARAT_RUNTIME_INVALID_HANDLE) return -1;
    if (count > 0 && !buf) return -1;

    // TODO: Stage 3 integration needed - Perform IPC request (FS_WRITE) to the resource capability handle.
    return -1; // Unimplemented
}

int bharat_runtime_close(bharat_runtime_handle_t handle) {
    if (handle == BHARAT_RUNTIME_INVALID_HANDLE) return -1;

    // TODO: Stage 3 integration needed - Perform IPC request (FS_CLOSE) to drop capability handle.
    return -1; // Unimplemented
}
