#ifndef SERVICEMGR_H
#define SERVICEMGR_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/ipc/ipc.h>

#define MAX_SERVICES 64
#define MAX_RESTART_COUNT 3
#define HEARTBEAT_TIMEOUT 5000 // ms

typedef struct {
    uint32_t service_id;
    char name[32];
    uint32_t required_caps;
    sm_service_state_t state;
    uint32_t restart_count;
    uint32_t last_heartbeat;
    bool in_use;
} service_entry_t;

void servicemgr_init(void);
void servicemgr_loop(bharat_ipc_endpoint_t endpoint);
int32_t servicemgr_handle_register(const sm_req_register_t *req, sm_resp_register_t *resp);
int32_t servicemgr_handle_start(const sm_req_start_t *req, sm_resp_start_t *resp);
int32_t servicemgr_handle_stop(const sm_req_stop_t *req, sm_resp_stop_t *resp);
int32_t servicemgr_handle_query(const sm_req_query_t *req, sm_resp_query_t *resp);
int32_t servicemgr_handle_heartbeat(const sm_req_heartbeat_t *req, sm_resp_heartbeat_t *resp);
void servicemgr_check_health(uint32_t current_time);

#endif // SERVICEMGR_H
