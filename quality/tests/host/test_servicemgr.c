#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../core/services/servicemgr/servicemgr.h"
#include <bharat/uapi/ipc/status.h>

void test_servicemgr_lifecycle(void) {
    servicemgr_init();

    sm_req_register_t req_reg = { .service_name = "test_service", .required_caps = 0 };
    sm_resp_register_t resp_reg;
    int32_t status = servicemgr_handle_register(&req_reg, &resp_reg);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp_reg.service_id == 1);

    sm_req_start_t req_start = { .service_id = 1 };
    sm_resp_start_t resp_start;
    status = servicemgr_handle_start(&req_start, &resp_start);
    assert(status == BHARAT_IPC_STATUS_OK);

    sm_req_query_t req_query = { .service_id = 1 };
    sm_resp_query_t resp_query;
    status = servicemgr_handle_query(&req_query, &resp_query);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp_query.state == SM_STATE_STARTING);

    sm_req_heartbeat_t req_hb = { .service_id = 1, .timestamp = 1000 };
    sm_resp_heartbeat_t resp_hb;
    status = servicemgr_handle_heartbeat(&req_hb, &resp_hb);
    assert(status == BHARAT_IPC_STATUS_OK);

    status = servicemgr_handle_query(&req_query, &resp_query);
    assert(resp_query.state == SM_STATE_RUNNING);

    // Test heartbeat timeout logic
    servicemgr_check_health(7000); // 1000 + 5000 + 1000 -> crashed and restarted

    status = servicemgr_handle_query(&req_query, &resp_query);
    assert(resp_query.state == SM_STATE_STARTING); // restarted once
    assert(resp_query.restart_count == 1);

    printf("test_servicemgr passed\n");
}

int main(void) {
    test_servicemgr_lifecycle();
    return 0;
}
