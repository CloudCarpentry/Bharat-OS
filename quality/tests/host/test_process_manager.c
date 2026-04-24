#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../services/process_manager/process_manager.h"
#include <bharat/uapi/ipc/status.h>

void test_process_lifecycle(void) {
    process_manager_init();

    pm_req_create_t req_create = { .executable_id = 100, .priority = 10 };
    pm_resp_create_t resp_create;
    int32_t status = process_manager_handle_create(&req_create, &resp_create);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp_create.process_id == 1);

    pm_req_query_t req_query = { .process_id = 1 };
    pm_resp_query_t resp_query;
    status = process_manager_handle_query(&req_query, &resp_query);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp_query.state == PM_STATE_CREATED);

    pm_req_start_t req_start = { .process_id = 1 };
    pm_resp_start_t resp_start;
    status = process_manager_handle_start(&req_start, &resp_start);
    assert(status == BHARAT_IPC_STATUS_OK);

    status = process_manager_handle_query(&req_query, &resp_query);
    assert(resp_query.state == PM_STATE_RUNNING);

    pm_req_stop_t req_stop = { .process_id = 1 };
    pm_resp_stop_t resp_stop;
    status = process_manager_handle_stop(&req_stop, &resp_stop);
    assert(status == BHARAT_IPC_STATUS_OK);

    status = process_manager_handle_query(&req_query, &resp_query);
    assert(resp_query.state == PM_STATE_STOPPING);

    printf("test_process_manager passed\n");
}

int main(void) {
    test_process_lifecycle();
    return 0;
}
