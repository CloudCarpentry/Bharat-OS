#ifndef BHARAT_UAPI_SERVICEMGR_CONTRACT_H
#define BHARAT_UAPI_SERVICEMGR_CONTRACT_H

#include <stdint.h>
#include <bharat/uapi/ipc/contract.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SERVICEMGR_SERVICE_ID 0x00010003

typedef enum {
    SM_OP_REGISTER = 1,
    SM_OP_START    = 2,
    SM_OP_STOP     = 3,
    SM_OP_QUERY    = 4,
    SM_OP_HEARTBEAT= 5
} sm_opcode_t;

typedef enum {
    SM_STATE_UNKNOWN = 0,
    SM_STATE_UNREGISTERED,
    SM_STATE_STOPPED,
    SM_STATE_STARTING,
    SM_STATE_RUNNING,
    SM_STATE_CRASHED,
    SM_STATE_BACKOFF
} sm_service_state_t;

// SM_OP_REGISTER Request
typedef struct {
    char service_name[32];
    uint32_t required_caps;
} sm_req_register_t;

// SM_OP_REGISTER Response
typedef struct {
    uint32_t service_id;
    int32_t status;
} sm_resp_register_t;

// SM_OP_START Request
typedef struct {
    uint32_t service_id;
} sm_req_start_t;

// SM_OP_START Response
typedef struct {
    int32_t status;
} sm_resp_start_t;

// SM_OP_STOP Request
typedef struct {
    uint32_t service_id;
} sm_req_stop_t;

// SM_OP_STOP Response
typedef struct {
    int32_t status;
} sm_resp_stop_t;

// SM_OP_QUERY Request
typedef struct {
    uint32_t service_id;
} sm_req_query_t;

// SM_OP_QUERY Response
typedef struct {
    uint32_t service_id;
    sm_service_state_t state;
    uint32_t restart_count;
    int32_t status;
} sm_resp_query_t;

// SM_OP_HEARTBEAT Request
typedef struct {
    uint32_t service_id;
    uint32_t timestamp;
} sm_req_heartbeat_t;

// SM_OP_HEARTBEAT Response
typedef struct {
    int32_t status;
} sm_resp_heartbeat_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_SERVICEMGR_CONTRACT_H
