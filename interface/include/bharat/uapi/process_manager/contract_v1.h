#ifndef BHARAT_UAPI_PROCESS_MANAGER_CONTRACT_V1_H
#define BHARAT_UAPI_PROCESS_MANAGER_CONTRACT_V1_H

#include <stdint.h>
#include <bharat/uapi/ipc/contract.h>
#include <uapi/handle/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BH_PM_INTERFACE_VERSION_V1 1U

typedef enum {
    BH_PM_OP_SPAWN_V1             = 0x10,
    BH_PM_OP_QUERY_V1             = 0x11,
    BH_PM_OP_REQUEST_TERMINATE_V1  = 0x12,
    BH_PM_OP_WAIT_V1              = 0x13,
    BH_PM_OP_REAP_V1              = 0x14,
} bh_pm_opcode_v1_t;

typedef enum {
    BH_PM_STATE_FREE_V1 = 0,
    BH_PM_STATE_RESERVED_V1,
    BH_PM_STATE_PROCESS_CREATED_V1,
    BH_PM_STATE_SPACE_CREATED_V1,
    BH_PM_STATE_IMAGE_REALIZED_V1,
    BH_PM_STATE_THREAD_CREATED_V1,
    BH_PM_STATE_READY_V1,
    BH_PM_STATE_RUNNING_V1,
    BH_PM_STATE_TERMINATE_REQUESTED_V1,
    BH_PM_STATE_EXITED_V1, // Zombie
    BH_PM_STATE_REAPED_V1,
    BH_PM_STATE_FAILED_V1
} bh_pm_state_v1_t;

typedef uint64_t bh_object_handle_t;
typedef bh_object_handle_t bh_pm_handle_t;
typedef bh_object_handle_t bh_vm_space_handle_t;
typedef bh_object_handle_t bh_vm_region_handle_t;

#define BH_PM_WAIT_NONBLOCK       (1U << 0)
#define BH_PM_WAIT_UNTIL_DEADLINE (1U << 1)

// bh_pm_spawn_request_v1_t matches recommended spawn request
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;

    uint64_t request_id;
    uint64_t executable_handle;

    bh_pm_handle_t parent_process;
    uint32_t priority;
    uint32_t affinity_mask;

    uint32_t memory_profile;
    uint32_t personality;
    uint32_t flags;
    uint32_t reserved;

    uint64_t stack_size;
    char process_name[32];
} bh_pm_spawn_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;

    int32_t status;
    uint32_t initial_state;

    bh_pm_handle_t process_handle;
    bh_vm_space_handle_t vm_space_handle;

    uint64_t kernel_process_id;
    uint64_t main_thread_id;
    uint64_t incarnation_id;
} bh_pm_spawn_response_v1_t;

// PM_OP_QUERY_V1 Request & Response
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_pm_handle_t process_handle;
} bh_pm_query_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
    uint32_t state; // bh_pm_state_v1_t
    uint64_t kernel_process_id;
    uint64_t main_thread_id;
    int32_t exit_code;
    uint32_t exit_reason;
} bh_pm_query_response_v1_t;

// REQUEST_TERMINATE Request & Response
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_pm_handle_t process_handle;
} bh_pm_terminate_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
} bh_pm_terminate_response_v1_t;

// WAIT Request & Response
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_pm_handle_t process_handle;
    uint32_t wait_flags; // BH_PM_WAIT_NONBLOCK / BH_PM_WAIT_UNTIL_DEADLINE
    uint32_t timeout_ms;
} bh_pm_wait_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
    int32_t exit_code;
    uint32_t exit_reason;
} bh_pm_wait_response_v1_t;

// REAP Request & Response
typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    bh_pm_handle_t process_handle;
} bh_pm_reap_request_v1_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    int32_t status;
} bh_pm_reap_response_v1_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_PROCESS_MANAGER_CONTRACT_V1_H
