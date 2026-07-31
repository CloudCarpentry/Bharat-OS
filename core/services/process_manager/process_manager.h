#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/process_manager/contract.h>
#include <bharat/ipc/ipc.h>
#include <bharat/elf/elf_load_plan.h>
#include <handle_table.h>

#define MAX_PROCESSES 128
#define MAX_EXECUTABLES 8

// Legacy Process Entry
typedef struct {
    uint32_t process_id;
    uint32_t executable_id;
    uint32_t priority;
    pm_process_state_t state;
    bool in_use;
} process_entry_t;

// v1 Kernel/VM adapter types
typedef struct {
    char name[32];
    uint32_t priority;
} bh_pm_kernel_create_req_t;

typedef struct {
    uint64_t pid;
} bh_pm_kernel_process_t;

typedef struct {
    uint64_t space_id;
} bh_vm_kernel_space_t;

typedef struct {
    uint64_t main_thread_id;
    uint64_t entry_point;
} bh_pm_kernel_image_result_t;

typedef struct {
    void *ctx;

    int32_t (*create_process)(void *ctx, const bh_pm_kernel_create_req_t *req,
                              bh_pm_kernel_process_t *out_proc);

    int32_t (*create_vm_space)(void *ctx, bh_pm_kernel_process_t *proc,
                               uint32_t memory_profile,
                               bh_vm_kernel_space_t *out_space);

    int32_t (*realize_image)(void *ctx, bh_pm_kernel_process_t *proc,
                             bh_vm_kernel_space_t *space,
                             const bh_user_image_plan_v1_t *plan,
                             bh_pm_kernel_image_result_t *out_res);

    int32_t (*start_process)(void *ctx, bh_pm_kernel_process_t *proc);

    int32_t (*request_terminate)(void *ctx, bh_pm_kernel_process_t *proc);

    int32_t (*reap_process)(void *ctx, bh_pm_kernel_process_t *proc);
} bh_pm_kernel_ops_t;

// v1 Process Entry
typedef struct {
    bh_pm_handle_t process_handle;
    bh_vm_space_handle_t vm_space_handle;
    uint64_t kernel_process_id;
    uint64_t main_thread_id;
    uint64_t incarnation_id;

    uint32_t state; // bh_pm_state_v1_t
    int32_t exit_code;
    uint32_t exit_reason;
    char name[32];

    uint64_t executable_handle;
    uint32_t priority;
    uint32_t affinity_mask;
    uint32_t memory_profile;

    // Waiter tracking for WAIT
    bool has_waiter;
    uint32_t waiter_flags;
    uint32_t waiter_timeout_ms;
} bh_pm_process_v1_t;

// Executable image registration
typedef struct {
    uint64_t handle;
    const uint8_t *bytes;
    size_t size;
    bool in_use;
} bh_pm_executable_image_t;

// Process Manager Global Interface
void process_manager_init(void);
void process_manager_loop(bharat_ipc_endpoint_t endpoint);
int32_t process_manager_handle_create(const pm_req_create_t *req, pm_resp_create_t *resp);
int32_t process_manager_handle_start(const pm_req_start_t *req, pm_resp_start_t *resp);
int32_t process_manager_handle_stop(const pm_req_stop_t *req, pm_resp_stop_t *resp);
int32_t process_manager_handle_query(const pm_req_query_t *req, pm_resp_query_t *resp);
int32_t process_manager_authorize(uint32_t opcode, const void *req, bharat_cap_handle_t caller_cap);

// v1 Process Manager Interfaces
void bh_pm_set_kernel_ops(const bh_pm_kernel_ops_t *ops);
void bh_pm_set_failure_injection(int fail_stage); // 0 = none, 1-5 represent spawn phases
int bh_pm_register_executable(uint64_t handle, const uint8_t *bytes, size_t size);
int bh_pm_get_active_count(void);

// Event receivers/handlers for v1 (exposed for direct transaction host testing)
int32_t bh_pm_handle_spawn_v1(const bh_pm_spawn_request_v1_t *req, bh_pm_spawn_response_v1_t *resp);
int32_t bh_pm_handle_query_v1(const bh_pm_query_request_v1_t *req, bh_pm_query_response_v1_t *resp);
int32_t bh_pm_handle_terminate_v1(const bh_pm_terminate_request_v1_t *req, bh_pm_terminate_response_v1_t *resp);
int32_t bh_pm_handle_wait_v1(const bh_pm_wait_request_v1_t *req, bh_pm_wait_response_v1_t *resp);
int32_t bh_pm_handle_reap_v1(const bh_pm_reap_request_v1_t *req, bh_pm_reap_response_v1_t *resp);

// Async kernel notifier
void bh_pm_notify_exit_v1(uint64_t kernel_process_id, int32_t exit_code, uint32_t exit_reason);

#endif // PROCESS_MANAGER_H
