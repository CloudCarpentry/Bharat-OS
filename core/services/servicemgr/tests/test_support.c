#include "test_support.h"
#include <string.h>
#include <stdlib.h>
uint64_t test_now;
int32_t test_spawn_status, test_stop_status;
uint32_t test_process_state, test_spawn_count;
static struct { uint32_t ep, len; unsigned char bytes[128]; bool used; } packets[16];
void bharat_runtime_log(const char *msg) { (void)msg; }
int bharat_sched_yield(void) { return 0; }

/* Mock the real syscall boundary, including the inline endpoint wrappers. */
long bh_syscall(long nr, long arg, long a2, long a3, long a4, long a5, long a6) {
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    if (!arg) return -1;
    if (nr == SYSCALL_ENDPOINT_SEND) {
        const bharat_sys_endpoint_send_args_t *a = (void *)(uintptr_t)arg;
        if (!a->send_cap || !a->payload_ptr || a->payload_len > 128 || a->reserved0) return -1;
        /* Authority-transfer mocks require a real fixture model; never infer it from headers. */
        if (a->cap_to_send) return -4;
        for (unsigned i = 0; i < 16; ++i) if (!packets[i].used) {
            packets[i].used = true; packets[i].ep = a->send_cap; packets[i].len = a->payload_len;
            memcpy(packets[i].bytes, (void *)(uintptr_t)a->payload_ptr, a->payload_len);
            return 0;
        }
        return -3;
    }
    if (nr == SYSCALL_ENDPOINT_RECEIVE) {
        const bharat_sys_endpoint_receive_args_t *a = (void *)(uintptr_t)arg;
        if (!a->recv_cap || !a->out_payload_ptr || !a->out_len_ptr) return -1;
        *(uint32_t *)(uintptr_t)a->out_len_ptr = 0;
        if (a->out_received_cap_ptr) *(uint32_t *)(uintptr_t)a->out_received_cap_ptr = 0;
        for (unsigned i = 0; i < 16; ++i) if (packets[i].used && packets[i].ep == a->recv_cap) {
            if (packets[i].len > a->out_payload_capacity) return -2;
            memcpy((void *)(uintptr_t)a->out_payload_ptr, packets[i].bytes, packets[i].len);
            *(uint32_t *)(uintptr_t)a->out_len_ptr = packets[i].len;
            packets[i].used = false;
            return 0;
        }
        return -8;
    }
    return -1;
}

static int32_t spawn(void *ctx, const bh_service_launch_spec_t *spec, bh_service_launch_result_t *out) {
    (void)ctx; assert(spec && out);
    if (test_spawn_status) return test_spawn_status;
    out->process_id = 100 + ++test_spawn_count; out->status = 0;
    test_process_state = PM_STATE_RUNNING;
    return 0;
}
static int32_t stop(void *ctx, uint32_t pid) { (void)ctx; assert(pid); return test_stop_status; }
static int32_t query(void *ctx, uint32_t pid, uint32_t *state) {
    (void)ctx; assert(pid); *state = test_process_state; return 0;
}
static int32_t clock_now(void *ctx, uint64_t *out) { (void)ctx; *out = test_now; return 0; }
const bh_service_launcher_ops_t test_launcher = {.spawn=spawn, .request_stop=stop, .query=query};
static const bh_servicemgr_clock_ops_t clock_ops = {.now_ns=clock_now};
bh_servicemgr_t *test_setup(void) {
    memset(packets, 0, sizeof(packets));
    test_now = 1000; test_spawn_status = test_stop_status = 0; test_spawn_count = 0;
    bh_servicemgr_dependencies_t deps = {.launcher=&test_launcher, .clock=&clock_ops};
    assert(servicemgr_init(&deps) == 0);
    bh_servicemgr_t *mgr = servicemgr_get_instance();
    mgr->boot_session_id = 42; mgr->my_endpoint = 0x200;
    return mgr;
}
void test_enqueue(uint32_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    assert(bharat_ipc_send(endpoint, header, payload) == 0);
}
void test_add_service(bool critical, uint32_t policy) {
    sm_req_register_t req = {.service_name="service1", .critical=critical, .restart_policy=policy};
    sm_resp_register_t resp;
    assert(servicemgr_handle_register(&req, &resp) == 0);
}
int32_t test_health(bh_service_instance_t *inst, uint32_t state) {
    sm_req_health_update_t req = {.struct_size=sizeof(req), .service_id=inst->service_id,
        .process_id=inst->process_id, .boot_session_id=42, .incarnation_id=inst->incarnation_id,
        .health_state=state, .heartbeat_sequence=inst->heartbeat_sequence+1,
        .timestamp_ticks=UINT64_MAX};
    sm_resp_health_update_t resp;
    return servicemgr_handle_health_update(&req, &resp, inst->process_id);
}
void test_ready(bh_service_instance_t *inst) {
    assert(test_health(inst, SM_HEALTH_STATE_BOUND) == 0);
    assert(inst->state == SM_STATE_STARTING);
    assert(test_health(inst, SM_HEALTH_STATE_READY) == 0);
    assert(inst->state == SM_STATE_RUNNING);
}
