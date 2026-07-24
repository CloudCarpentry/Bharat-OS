#include "../servicemgr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <bharat/uapi/ipc/status.h>

// Mock IPC
int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) { (void)endpoint; (void)header; (void)payload; return 0; }
int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size) { (void)endpoint; (void)header; (void)payload_buf; (void)max_size; return -1; }

void test_lifecycle(void) {
    servicemgr_init();

    const char *deps[] = {"service1"};
    bh_service_manifest_entry_t manifest[] = {
        {
            .name = "service1",
            .service_id = 1,
            .hard_deps_count = 0,
            .restart_policy = SM_RESTART_POLICY_ALWAYS,
            .critical = true
        },
        {
            .name = "service2",
            .service_id = 2,
            .hard_deps = deps,
            .hard_deps_count = 1,
            .restart_policy = SM_RESTART_POLICY_ALWAYS,
            .critical = false
        }
    };

    assert(servicemgr_load_manifest(manifest, 2) == BHARAT_IPC_STATUS_OK);
    assert(servicemgr_validate_dependencies() == BHARAT_IPC_STATUS_OK);

    sm_req_query_t query_req = {.service_id = 1};
    sm_resp_query_t query_resp;

    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_CREATED);

    servicemgr_start_all();

    servicemgr_handle_query(&query_req, &query_resp);
    printf("State: %d, SM_STATE_STARTING: %d, incarnation_id: %lu\n", (int)query_resp.state, (int)SM_STATE_STARTING, (unsigned long)query_resp.incarnation_id);
    assert((int)query_resp.state == (int)SM_STATE_STARTING);
    assert(query_resp.incarnation_id == 1);

    // Signal ready
    sm_req_heartbeat_t ready_req = {.service_id = 1, .incarnation_id = 1};
    sm_resp_heartbeat_t ready_resp;
    assert(servicemgr_handle_signal_ready(&ready_req, &ready_resp) == BHARAT_IPC_STATUS_OK);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RUNNING);

    // Test Heartbeat and Timeout
    sm_req_heartbeat_t hb_req = {
        .service_id = 1,
        .incarnation_id = 1,
        .timestamp_ticks = 1000
    };
    sm_resp_heartbeat_t hb_resp;
    assert(servicemgr_handle_heartbeat(&hb_req, &hb_resp) == BHARAT_IPC_STATUS_OK);

    // Advance time beyond timeout
    servicemgr_check_health(1000 + HEARTBEAT_TIMEOUT_MS + 1);

    servicemgr_handle_query(&query_req, &query_resp);
    printf("State after timeout: %d\n", (int)query_resp.state);
    // Should be RESTART_BACKOFF because policy is ALWAYS
    assert(query_resp.state == SM_STATE_RESTART_BACKOFF);
    assert(query_resp.restart_count == 1);

    // Advance time beyond backoff
    servicemgr_check_health(1000 + HEARTBEAT_TIMEOUT_MS + 1 + DEFAULT_INITIAL_BACKOFF_MS + 1);

    servicemgr_handle_query(&query_req, &query_resp);
    printf("State after backoff: %d\n", (int)query_resp.state);
    assert(query_resp.state == SM_STATE_STARTING);

    // Signal ready for second incarnation
    hb_req.incarnation_id = 2;
    assert(servicemgr_handle_signal_ready(&hb_req, &ready_resp) == BHARAT_IPC_STATUS_OK);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RUNNING);
    assert(query_resp.incarnation_id == 2);

    // Stale incarnation heartbeat
    hb_req.incarnation_id = 1;
    assert(servicemgr_handle_heartbeat(&hb_req, &hb_resp) == BHARAT_IPC_STATUS_ERR_PERM);

    printf("test_lifecycle passed\n");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    test_lifecycle();
    return 0;
}
