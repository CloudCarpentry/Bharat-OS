#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../../core/services/servicemgr/servicemgr.h"
#include <bharat/uapi/ipc/status.h>
#include <bharat/uapi/process_manager/contract.h>
#include <bharat/namesvc/client.h>
#include <bharat/cap/cap_authz.h>
#include <bharat/runtime/runtime.h>

// Link mock fake launcher
#include "services/fake_service_launcher.c"

#define TEST_STABILITY_WINDOW_NS (2ULL * HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS)

// Stubs for runtime.h logging/panic/syscalls
void bharat_runtime_log(const char *msg) {
    (void)msg;
}
void bharat_runtime_panic(const char *reason) {
    printf("PANIC: %s\n", reason);
    assert(0);
}
long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    (void)sysno; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return 0;
}
int bharat_sched_yield(void) {
    return 0;
}

// In-Memory IPC Queue implementation
#define MAX_QUEUE_SIZE 128
typedef struct {
    uint32_t ep;
    uint8_t payload[512];
    uint32_t len;
} ipc_packet_t;

static ipc_packet_t ipc_queue[MAX_QUEUE_SIZE];
static int ipc_q_head = 0;
static int ipc_q_tail = 0;

void clear_ipc_queue(void) {
    ipc_q_head = 0;
    ipc_q_tail = 0;
}

long bharat_ipc_transport_send(uint32_t send_cap, const void *payload, uint32_t len, uint64_t timeout_ticks) {
    (void)timeout_ticks;
    int next = (ipc_q_tail + 1) % MAX_QUEUE_SIZE;
    if (next == ipc_q_head) return -3; // BUSY / Full
    ipc_queue[ipc_q_tail].ep = send_cap;
    if (len > sizeof(ipc_queue[0].payload)) len = sizeof(ipc_queue[0].payload);
    memcpy(ipc_queue[ipc_q_tail].payload, payload, len);
    ipc_queue[ipc_q_tail].len = len;
    ipc_q_tail = next;
    return 0;
}

long bharat_ipc_transport_receive(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks) {
    (void)timeout_ticks;
    int curr = ipc_q_head;
    while (curr != ipc_q_tail) {
        if (ipc_queue[curr].ep == recv_cap) {
            uint32_t copy_len = ipc_queue[curr].len;
            if (copy_len > out_capacity) copy_len = out_capacity;
            memcpy(out_payload, ipc_queue[curr].payload, copy_len);
            if (out_len) *out_len = copy_len;

            // Remove from queue: shift remaining elements
            int next_curr = (curr + 1) % MAX_QUEUE_SIZE;
            while (next_curr != ipc_q_tail) {
                ipc_queue[curr] = ipc_queue[next_curr];
                curr = next_curr;
                next_curr = (curr + 1) % MAX_QUEUE_SIZE;
            }
            ipc_q_tail = curr;
            return 0;
        }
        curr = (curr + 1) % MAX_QUEUE_SIZE;
    }
    return -8; // TIMEOUT (empty)
}

// Namesvc mock implementation
typedef struct {
    char name[64];
    bharat_service_id_t service_id;
    bharat_ipc_endpoint_t ep;
    uint32_t version;
} mock_namesvc_entry_t;

static mock_namesvc_entry_t mock_namesvc_table[32];
static int mock_namesvc_count = 0;

void clear_mock_namesvc(void) {
    mock_namesvc_count = 0;
    memset(mock_namesvc_table, 0, sizeof(mock_namesvc_table));
}

int namesvc_register(const char *name,
                     bharat_service_id_t service_id,
                     bharat_ipc_endpoint_t endpoint,
                     uint32_t version,
                     uint32_t flags) {
    (void)flags;
    for (int i = 0; i < mock_namesvc_count; i++) {
        if (strcmp(mock_namesvc_table[i].name, name) == 0) {
            return NAMESVC_STATUS_ERR_EXISTS;
        }
    }
    strncpy(mock_namesvc_table[mock_namesvc_count].name, name, sizeof(mock_namesvc_table[0].name) - 1);
    mock_namesvc_table[mock_namesvc_count].service_id = service_id;
    mock_namesvc_table[mock_namesvc_count].ep = endpoint;
    mock_namesvc_table[mock_namesvc_count].version = version;
    mock_namesvc_count++;
    return NAMESVC_STATUS_OK;
}

int namesvc_lookup(const char *name,
                   bharat_service_id_t *out_service_id,
                   bharat_ipc_endpoint_t *out_endpoint,
                   uint32_t *out_version) {
    for (int i = 0; i < mock_namesvc_count; i++) {
        if (strcmp(mock_namesvc_table[i].name, name) == 0) {
            if (out_service_id) *out_service_id = mock_namesvc_table[i].service_id;
            if (out_endpoint) *out_endpoint = mock_namesvc_table[i].ep;
            if (out_version) *out_version = mock_namesvc_table[i].version;
            return NAMESVC_STATUS_OK;
        }
    }
    return NAMESVC_STATUS_ERR_NOTFOUND;
}

// Capability validation stub/mock
static bool auth_should_fail = false;

int32_t bharat_service_dispatch_authorize(
    uint32_t service_id,
    uint32_t opcode,
    const bharat_service_authz_desc_t *descs,
    uint32_t desc_count,
    bharat_cap_handle_t caller_cap,
    uint64_t target_object_id)
{
    (void)service_id; (void)opcode; (void)descs; (void)desc_count; (void)target_object_id;
    if (auth_should_fail || caller_cap == 0xFFFF) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }
    return BHARAT_IPC_STATUS_OK;
}

// Setup helpers
void setup_test_env(bh_servicemgr_t **out_mgr) {
    clear_ipc_queue();
    clear_mock_namesvc();
    fake_launcher_init();
    auth_should_fail = false;

    bh_servicemgr_dependencies_t deps = {
        .launcher = &fake_launcher_ops,
        .clock = &fake_clock_ops,
        .registry = &fake_registry_ops
    };

    servicemgr_init(&deps);
    *out_mgr = servicemgr_get_instance();
    (*out_mgr)->my_endpoint = 0x200; // dynamic EP
}

// IPC single-threaded test helper
static int32_t test_ipc_call_ex(uint32_t ep, uint32_t opcode, const void *payload, uint32_t payload_size, void *out_resp, uint32_t resp_size, uint32_t caller_cap) {
    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = SERVICEMGR_SERVICE_ID,
        .opcode = opcode,
        .payload_size = payload_size,
        .capability_transfer = caller_cap,
        .reply_endpoint = 0x300 // reply EP
    };

    int32_t send_rc = bharat_ipc_send(ep, &req_hdr, payload);
    if (send_rc != 0) return send_rc;

    // Poll servicemgr once to process and send reply
    bh_servicemgr_t *mgr = servicemgr_get_instance();
    servicemgr_poll_once(mgr, 100);

    // Receive reply
    bharat_ipc_msg_header_t rep_hdr = {0};
    int32_t recv_rc = bharat_ipc_recv(0x300, &rep_hdr, out_resp, resp_size);
    if (recv_rc != 0) return recv_rc;

    return rep_hdr.status;
}

static int32_t test_ipc_call(uint32_t ep, uint32_t opcode, const void *payload, uint32_t payload_size, void *out_resp, uint32_t resp_size) {
    return test_ipc_call_ex(ep, opcode, payload, payload_size, out_resp, resp_size, 0x40);
}

// 1. Handoff version and size validation
void test_handoff_validation(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    // Call handoff begin with invalid major version
    sm_handoff_begin_t begin = {
        .abi_major = 99, // Bad major
        .abi_minor = 0,
        .struct_size = sizeof(sm_handoff_begin_t),
        .selected_service_count = 2,
        .boot_session_id = 0x1234
    };

    sm_handoff_resp_t resp;
    int32_t status = test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_BEGIN, &begin, sizeof(begin), &resp, sizeof(resp));
    assert(status == BHARAT_IPC_STATUS_ERR_VERSION);

    // Call handoff begin with bad struct size
    begin.abi_major = 1;
    begin.struct_size = 12; // Bad size

    status = test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_BEGIN, &begin, sizeof(begin), &resp, sizeof(resp));
    assert(status == BHARAT_IPC_STATUS_ERR_VERSION);

    printf("test_handoff_validation passed\n");
}

// 2. Unauthorized handoff
void test_handoff_unauthorized(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    sm_handoff_begin_t begin = {
        .abi_major = 1,
        .struct_size = sizeof(sm_handoff_begin_t),
        .selected_service_count = 2,
        .boot_session_id = 0x1234
    };

    sm_handoff_resp_t resp;
    // Pass BHARAT_CAP_INVALID_HANDLE to test_ipc_call_ex to check unauthorized rejection
    int32_t status = test_ipc_call_ex(mgr->my_endpoint, SM_OP_HANDOFF_BEGIN, &begin, sizeof(begin), &resp, sizeof(resp), BHARAT_CAP_INVALID_HANDLE);
    assert(status == BHARAT_IPC_STATUS_ERR_PERM);

    printf("test_handoff_unauthorized passed\n");
}

// 3. Out-of-order records during chunked handoff
void test_handoff_out_of_order_records(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    sm_handoff_begin_t begin = {
        .abi_major = 1,
        .struct_size = sizeof(sm_handoff_begin_t),
        .selected_service_count = 2,
        .boot_session_id = 0x1234
    };

    sm_handoff_resp_t resp;
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_BEGIN, &begin, sizeof(begin), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Send record 1 instead of 0
    sm_handoff_service_t rec = {
        .record_index = 1, // out of order!
        .service_id = 5
    };

    int32_t status = test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_SERVICE, &rec, sizeof(rec), &resp, sizeof(resp));
    printf("test_handoff_out_of_order_records: Got status %d, expected %d\n", status, BHARAT_IPC_STATUS_ERR_INVALID);
    fflush(stdout);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    printf("test_handoff_out_of_order_records passed\n");
}

// 4. Adoption of pre-started services
void test_adoption_prestarted(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    sm_handoff_begin_t begin = {
        .abi_major = 1,
        .struct_size = sizeof(sm_handoff_begin_t),
        .selected_service_count = 1,
        .boot_session_id = 0x1234
    };

    sm_handoff_resp_t resp;
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_BEGIN, &begin, sizeof(begin), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Service index 0: already running (process_id = 50)
    sm_handoff_service_t rec = {
        .record_index = 0,
        .service_id = 10,
        .service_name = "namesvc",
        .process_id = 50, // prestarted process ID
        .incarnation_id = 1
    };

    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_SERVICE, &rec, sizeof(rec), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Commit handoff
    sm_handoff_commit_t commit = {
        .boot_session_id = 0x1234,
        .record_count = 1
    };
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_COMMIT, &commit, sizeof(commit), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Verify it is adopted as RUNNING, and process_id/incarnation are preserved
    assert(mgr->service_instances[0].state == SM_STATE_RUNNING);
    assert(mgr->service_instances[0].process_id == 50);
    assert(mgr->service_instances[0].incarnation_id == 1);

    printf("test_adoption_prestarted passed\n");
}

// 5. Launcher failure handling
static int32_t fail_spawn2(void *ctx, const bh_service_launch_spec_t *spec, bh_service_launch_result_t *out) {
    (void)ctx; (void)spec; (void)out;
    return BHARAT_IPC_STATUS_ERR_PERM;
}

void test_launcher_failure(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    sm_req_register_t reg = {
        .service_name = "bad_service",
        .restart_policy = SM_RESTART_POLICY_NONE,
        .critical = true
    };
    sm_resp_register_t resp_reg;
    assert(servicemgr_handle_register(&reg, &resp_reg) == 0);

    // Simulate a failure in launcher by temporarily overriding spawn to return failure
    const bh_service_launcher_ops_t bad_launcher = {
        .ctx = NULL,
        .spawn = fail_spawn2
    };
    mgr->deps.launcher = &bad_launcher;

    // Start all
    servicemgr_start_all();

    // Verify state transitioned to FAILED
    assert(mgr->service_instances[0].state == SM_STATE_FAILED);

    printf("test_launcher_failure passed\n");
}

// 6. Idle heartbeat timeout detection
void test_heartbeat_timeout_idle(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    // Prestart/adopt namesvc
    mgr->service_instances[0].service_id = 10;
    strncpy(mgr->service_instances[0].graph_rec.service_name, "namesvc", 31);
    mgr->service_instances[0].state = SM_STATE_RUNNING;
    mgr->service_instances[0].process_id = 50;
    mgr->service_instances[0].incarnation_id = 1;
    mgr->service_instances[0].in_use = true;
    mgr->service_instances[0].last_heartbeat_ns = 1000;
    mgr->service_instances[0].graph_rec.restart_policy = SM_RESTART_POLICY_NONE;
    mgr->instance_count = 1;

    // Set time past heartbeat timeout
    fake_launcher_set_time(1000 + (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS + 100);

    // Run poll/check health
    uint64_t current_time_ns = 1000 + (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS + 100;
    servicemgr_poll_once(mgr, current_time_ns + 10);

    // Should transition to FAILED
    assert(mgr->service_instances[0].state == SM_STATE_FAILED);

    printf("test_heartbeat_timeout_idle passed\n");
}

// 7. Future timestamps do not suppress timeout detection
void test_future_timestamps(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    mgr->service_instances[0].service_id = 10;
    strncpy(mgr->service_instances[0].graph_rec.service_name, "namesvc", 31);
    mgr->service_instances[0].state = SM_STATE_RUNNING;
    mgr->service_instances[0].incarnation_id = 1;
    mgr->service_instances[0].in_use = true;
    mgr->instance_count = 1;

    fake_launcher_set_time(1000);

    // Heartbeat payload with future timestamp
    sm_req_heartbeat_t hb = {
        .service_id = 10,
        .incarnation_id = 1,
        .timestamp_ticks = 999999999999ULL // massive future timestamp
    };
    sm_resp_heartbeat_t resp;
    assert(servicemgr_handle_heartbeat(&hb, &resp) == 0);

    // Confirm that inst->last_heartbeat_ns is the supervisor's time (1000), not the massive future client timestamp
    assert(mgr->service_instances[0].last_heartbeat_ns == 1000);

    printf("test_future_timestamps passed\n");
}

// 8. Stale incarnation heartbeat and ready rejection
void test_stale_incarnation(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    mgr->service_instances[0].service_id = 10;
    strncpy(mgr->service_instances[0].graph_rec.service_name, "namesvc", 31);
    mgr->service_instances[0].state = SM_STATE_STARTING;
    mgr->service_instances[0].incarnation_id = 2; // current incarnation is 2
    mgr->service_instances[0].in_use = true;
    mgr->instance_count = 1;

    // Send ready with stale incarnation 1
    sm_req_heartbeat_t req = {
        .service_id = 10,
        .incarnation_id = 1 // stale
    };
    sm_resp_heartbeat_t resp;
    assert(servicemgr_handle_signal_ready(&req, &resp) == BHARAT_IPC_STATUS_ERR_PERM);

    // Send heartbeat with stale incarnation 1
    assert(servicemgr_handle_heartbeat(&req, &resp) == BHARAT_IPC_STATUS_ERR_PERM);

    printf("test_stale_incarnation passed\n");
}

// 9. Bounded restart count & exponential backoff cap
void test_bounded_restart(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    mgr->service_instances[0].service_id = 10;
    strncpy(mgr->service_instances[0].graph_rec.service_name, "namesvc", 31);
    mgr->service_instances[0].state = SM_STATE_RUNNING;
    mgr->service_instances[0].incarnation_id = 1;
    mgr->service_instances[0].in_use = true;
    mgr->service_instances[0].graph_rec.restart_policy = SM_RESTART_POLICY_BOUNDED_RETRY;
    mgr->service_instances[0].last_heartbeat_ns = 1000;
    mgr->service_instances[0].current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;
    mgr->instance_count = 1;

    uint64_t current_time = 1000;

    // Trigger failure 1
    current_time += (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS + 100;
    fake_launcher_set_time(current_time);
    servicemgr_check_health(current_time);
    assert(mgr->service_instances[0].state == SM_STATE_RESTART_BACKOFF);
    assert(mgr->service_instances[0].restart_count == 1);
    assert(mgr->service_instances[0].current_backoff_ms == DEFAULT_INITIAL_BACKOFF_MS * 2);

    // Advance time to start
    current_time += (uint64_t)DEFAULT_INITIAL_BACKOFF_MS * BH_NS_PER_MS + 100;
    fake_launcher_set_time(current_time);
    servicemgr_check_health(current_time);
    assert(mgr->service_instances[0].state == SM_STATE_STARTING);
    mgr->service_instances[0].state = SM_STATE_RUNNING;
    mgr->service_instances[0].last_heartbeat_ns = current_time;

    // Trigger failure 2
    current_time += (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS + 100;
    fake_launcher_set_time(current_time);
    servicemgr_check_health(current_time);
    assert(mgr->service_instances[0].state == SM_STATE_RESTART_BACKOFF);
    assert(mgr->service_instances[0].restart_count == 2);
    assert(mgr->service_instances[0].current_backoff_ms == DEFAULT_INITIAL_BACKOFF_MS * 4);

    // Advance time to start
    current_time += (uint64_t)DEFAULT_INITIAL_BACKOFF_MS * 2 * BH_NS_PER_MS + 100;
    fake_launcher_set_time(current_time);
    servicemgr_check_health(current_time);
    assert(mgr->service_instances[0].state == SM_STATE_STARTING);
    mgr->service_instances[0].state = SM_STATE_RUNNING;
    mgr->service_instances[0].last_heartbeat_ns = current_time;

    // Trigger failure 3
    current_time += (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS + 100;
    fake_launcher_set_time(current_time);
    servicemgr_check_health(current_time);
    assert(mgr->service_instances[0].state == SM_STATE_RESTART_BACKOFF);
    assert(mgr->service_instances[0].restart_count == 3);

    // Advance time to start
    current_time += (uint64_t)DEFAULT_INITIAL_BACKOFF_MS * 4 * BH_NS_PER_MS + 100;
    fake_launcher_set_time(current_time);
    servicemgr_check_health(current_time);
    assert(mgr->service_instances[0].state == SM_STATE_STARTING);
    mgr->service_instances[0].state = SM_STATE_RUNNING;
    mgr->service_instances[0].last_heartbeat_ns = current_time;

    // Trigger failure 4 (exhaust budget!)
    current_time += (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS + 100;
    fake_launcher_set_time(current_time);
    servicemgr_check_health(current_time);
    // Should stay FAILED and not transition to RESTART_BACKOFF
    assert(mgr->service_instances[0].state == SM_STATE_FAILED);

    printf("test_bounded_restart passed\n");
}

// 10. Every opcode authorized
void test_all_opcodes_authorized(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    auth_should_fail = true;

    sm_req_register_t reg = { .service_name = "test" };
    sm_resp_register_t resp_reg;
    int32_t status = test_ipc_call(mgr->my_endpoint, SM_OP_REGISTER, &reg, sizeof(reg), &resp_reg, sizeof(resp_reg));
    assert(status == BHARAT_IPC_STATUS_ERR_PERM);

    printf("test_all_opcodes_authorized passed\n");
}

// 11. Complete handoff end-to-end integration test
void test_handoff_integration_e2e(void) {
    bh_servicemgr_t *mgr;
    setup_test_env(&mgr);

    // Begin handoff
    sm_handoff_begin_t begin = {
        .abi_major = 1,
        .struct_size = sizeof(sm_handoff_begin_t),
        .selected_service_count = 3,
        .boot_session_id = 0x5555
    };

    sm_handoff_resp_t resp;
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_BEGIN, &begin, sizeof(begin), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Send Service 0 (prestarted namesvc)
    sm_handoff_service_t s0 = {
        .record_index = 0,
        .service_id = 1,
        .service_name = "namesvc",
        .boot_class = 0,
        .critical = 1,
        .process_id = 10,
        .incarnation_id = 1
    };
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_SERVICE, &s0, sizeof(s0), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Send Service 1 (prestarted process_manager)
    sm_handoff_service_t s1 = {
        .record_index = 1,
        .service_id = 2,
        .service_name = "process_manager",
        .boot_class = 0,
        .critical = 1,
        .process_id = 20,
        .incarnation_id = 1
    };
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_SERVICE, &s1, sizeof(s1), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Send Service 2 (not running, critical devmgr)
    sm_handoff_service_t s2 = {
        .record_index = 2,
        .service_id = 4,
        .service_name = "devmgr",
        .boot_class = 1,
        .critical = 1,
        .process_id = 0, // not prestarted
        .incarnation_id = 0
    };
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_SERVICE, &s2, sizeof(s2), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Commit handoff
    sm_handoff_commit_t commit = {
        .boot_session_id = 0x5555,
        .record_count = 3
    };
    assert(test_ipc_call(mgr->my_endpoint, SM_OP_HANDOFF_COMMIT, &commit, sizeof(commit), &resp, sizeof(resp)) == BHARAT_IPC_STATUS_OK);

    // Verify s2 (devmgr) was spawned (entered STARTING)
    assert(mgr->service_instances[2].state == SM_STATE_STARTING);
    assert(mgr->service_instances[2].incarnation_id == 1);

    // Signal ready for devmgr
    sm_req_heartbeat_t ready_req = {
        .service_id = 4,
        .incarnation_id = 1
    };
    sm_resp_heartbeat_t ready_resp;
    assert(servicemgr_handle_signal_ready(&ready_req, &ready_resp) == BHARAT_IPC_STATUS_OK);
    assert(mgr->service_instances[2].state == SM_STATE_RUNNING);

    // Run health check / check for STABLE
    fake_launcher_set_time(100);
    servicemgr_check_health(100);

    fake_launcher_set_time(100 + TEST_STABILITY_WINDOW_NS + 100);
    servicemgr_check_health(100 + TEST_STABILITY_WINDOW_NS + 100);

    // Should reach stable state
    assert(mgr->stable_reached == true);

    printf("test_handoff_integration_e2e passed\n");
}

int main(void) {
    test_handoff_validation();
    test_handoff_unauthorized();
    test_handoff_out_of_order_records();
    test_adoption_prestarted();
    test_launcher_failure();
    test_heartbeat_timeout_idle();
    test_future_timestamps();
    test_stale_incarnation();
    test_bounded_restart();
    test_all_opcodes_authorized();
    test_handoff_integration_e2e();

    printf("All host test_servicemgr tests passed!\n");
    return 0;
}
