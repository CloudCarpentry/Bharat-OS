#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/process_manager/contract.h>
#include <bharat/ipc/ipc.h>

#define MAX_PROCESSES 128

typedef struct {
    uint32_t process_id;
    uint32_t executable_id;
    uint32_t priority;
    pm_process_state_t state;
    bool in_use;
} process_entry_t;

void process_manager_init(void);
void process_manager_loop(bharat_ipc_endpoint_t endpoint);
int32_t process_manager_handle_create(const pm_req_create_t *req, pm_resp_create_t *resp);
int32_t process_manager_handle_start(const pm_req_start_t *req, pm_resp_start_t *resp);
int32_t process_manager_handle_stop(const pm_req_stop_t *req, pm_resp_stop_t *resp);
int32_t process_manager_handle_query(const pm_req_query_t *req, pm_resp_query_t *resp);

#endif // PROCESS_MANAGER_H
