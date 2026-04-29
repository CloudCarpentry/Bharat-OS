#include "../servicemgr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <bharat/uapi/ipc/status.h>

// Mock IPC
int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) { (void)endpoint; (void)header; (void)payload; return 0; }
int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size) { (void)endpoint; (void)header; (void)payload_buf; (void)max_size; return -1; }

void test_backoff(void) {
    printf("--- test_backoff ---\n");
    servicemgr_init();

    bh_service_manifest_entry_t manifest[] = {
        {
            .name = "service1",
            .service_id = 1,
            .restart_policy = SM_RESTART_POLICY_ALWAYS
        }
    };

    servicemgr_load_manifest(manifest, 1);
    servicemgr_start_all();

    sm_req_heartbeat_t hb_req = {.service_id = 1, .incarnation_id = 1, .timestamp_ticks = 100};
    sm_resp_heartbeat_t hb_resp;
    servicemgr_handle_signal_ready(&hb_req, &hb_resp);
    servicemgr_handle_heartbeat(&hb_req, &hb_resp);

    // Fail 1: Backoff should be 100ms
    servicemgr_check_health(100 + HEARTBEAT_TIMEOUT_MS + 1);
    sm_req_query_t query_req = {.service_id = 1};
    sm_resp_query_t query_resp;
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RESTART_BACKOFF);

    // 50ms later, still backoff
    servicemgr_check_health(100 + HEARTBEAT_TIMEOUT_MS + 51);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RESTART_BACKOFF);

    // 101ms later, should be STARTING
    servicemgr_check_health(100 + HEARTBEAT_TIMEOUT_MS + 101);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_STARTING);

    // Signal ready
    servicemgr_handle_signal_ready(&(sm_req_heartbeat_t){.service_id = 1, .incarnation_id = 2}, &hb_resp);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RUNNING);
    assert(query_resp.incarnation_id == 2);

    // Fail 2: Backoff should be 200ms
    servicemgr_handle_heartbeat(&(sm_req_heartbeat_t){.service_id = 1, .incarnation_id = 2, .timestamp_ticks = 10000}, &hb_resp);
    servicemgr_check_health(10000 + HEARTBEAT_TIMEOUT_MS + 1);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RESTART_BACKOFF);

    servicemgr_check_health(10000 + HEARTBEAT_TIMEOUT_MS + 150);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RESTART_BACKOFF);

    servicemgr_check_health(10000 + HEARTBEAT_TIMEOUT_MS + 201);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_STARTING);

    servicemgr_handle_signal_ready(&(sm_req_heartbeat_t){.service_id = 1, .incarnation_id = 3}, &hb_resp);
    servicemgr_handle_query(&query_req, &query_resp);
    assert(query_resp.state == SM_STATE_RUNNING);
    assert(query_resp.incarnation_id == 3);

    printf("test_backoff passed\n");
}

void test_dependencies(void) {
    printf("--- test_dependencies ---\n");
    servicemgr_init();

    const char *deps1[] = {"nonexistent"};
    const char *deps2[] = {"service1"};
    bh_service_manifest_entry_t manifest[] = {
        { .name = "service1", .service_id = 1, .hard_deps = deps1, .hard_deps_count = 1 },
        { .name = "service2", .service_id = 2, .hard_deps = deps2, .hard_deps_count = 1 }
    };

    servicemgr_load_manifest(manifest, 2);
    assert(servicemgr_validate_dependencies() == BHARAT_IPC_STATUS_ERR_NOT_FOUND);

    servicemgr_init();
    bh_service_manifest_entry_t manifest2[] = {
        { .name = "service1", .service_id = 1 },
        { .name = "service2", .service_id = 2, .hard_deps = deps2, .hard_deps_count = 1 }
    };
    servicemgr_load_manifest(manifest2, 2);
    assert(servicemgr_validate_dependencies() == BHARAT_IPC_STATUS_OK);

    servicemgr_start_all();
    sm_resp_query_t query_resp;
    servicemgr_handle_query(&(sm_req_query_t){.service_id = 1}, &query_resp);
    assert(query_resp.state == SM_STATE_STARTING);
    servicemgr_handle_query(&(sm_req_query_t){.service_id = 2}, &query_resp);
    assert(query_resp.state == SM_STATE_CREATED); // service2 can't start because service1 is not RUNNING

    // Make service1 RUNNING
    servicemgr_handle_signal_ready(&(sm_req_heartbeat_t){.service_id = 1, .incarnation_id = 1}, &(sm_resp_heartbeat_t){0});
    servicemgr_start_all();
    servicemgr_handle_query(&(sm_req_query_t){.service_id = 2}, &query_resp);
    assert(query_resp.state == SM_STATE_STARTING);

    printf("test_dependencies passed\n");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    test_backoff();
    test_dependencies();
    return 0;
}
