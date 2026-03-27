#ifndef BHARAT_UAPI_PROCESS_MANAGER_CONTRACT_H
#define BHARAT_UAPI_PROCESS_MANAGER_CONTRACT_H

#include <stdint.h>
#include <bharat/uapi/ipc/contract.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROCESS_MANAGER_SERVICE_ID 0x00010001

typedef enum {
    PM_OP_CREATE = 1,
    PM_OP_START  = 2,
    PM_OP_STOP   = 3,
    PM_OP_QUERY  = 4
} pm_opcode_t;

typedef enum {
    PM_STATE_UNKNOWN = 0,
    PM_STATE_CREATED,
    PM_STATE_READY,
    PM_STATE_RUNNING,
    PM_STATE_STOPPING,
    PM_STATE_EXITED,
    PM_STATE_FAILED
} pm_process_state_t;

// PM_OP_CREATE Request
typedef struct {
    uint32_t executable_id;
    uint32_t priority;
} pm_req_create_t;

// PM_OP_CREATE Response
typedef struct {
    uint32_t process_id;
    int32_t status;
} pm_resp_create_t;

// PM_OP_START Request
typedef struct {
    uint32_t process_id;
} pm_req_start_t;

// PM_OP_START Response
typedef struct {
    int32_t status;
} pm_resp_start_t;

// PM_OP_STOP Request
typedef struct {
    uint32_t process_id;
} pm_req_stop_t;

// PM_OP_STOP Response
typedef struct {
    int32_t status;
} pm_resp_stop_t;

// PM_OP_QUERY Request
typedef struct {
    uint32_t process_id;
} pm_req_query_t;

// PM_OP_QUERY Response
typedef struct {
    uint32_t process_id;
    pm_process_state_t state;
    int32_t status;
} pm_resp_query_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_PROCESS_MANAGER_CONTRACT_H
