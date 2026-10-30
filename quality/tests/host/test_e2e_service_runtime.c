#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "../../core/services/process_manager/process_manager.h"
#include "../../core/services/vm_manager/vm_manager.h"
#include "../../core/services/servicemgr/servicemgr.h"
#include <bharat/uapi/ipc/status.h>
#include <bharat/cap/cap_validate.h>

void bharat_runtime_log(const char *msg) { (void)msg; }
int bharat_sched_yield(void) { return 0; }

static uint64_t g_test_now_ns = 1000ULL * BH_NS_PER_MS;
static uint32_t g_next_pid = 100U;

static int32_t e2e_clock_now(void *ctx, uint64_t *out_now_ns) {
    (void)ctx;
    *out_now_ns = g_test_now_ns;
    return 0;
}

static int32_t e2e_launcher_spawn(void *ctx, const bh_service_launch_spec_t *spec, bh_service_launch_result_t *out) {
    (void)ctx;
    assert(spec && out);
    pm_req_create_t req_c = { .executable_id = spec->executable_id ? spec->executable_id : 1U, .priority = spec->priority ? spec->priority : 5U };
    pm_resp_create_t resp_c = {0};
    if (process_manager_handle_create(&req_c, &resp_c) == BHARAT_IPC_STATUS_OK) {
        pm_req_start_t req_s = { .process_id = resp_c.process_id };
        pm_resp_start_t resp_s = {0};
        assert(process_manager_handle_start(&req_s, &resp_s) == BHARAT_IPC_STATUS_OK);
        out->process_id = resp_c.process_id;
    } else {
        out->process_id = ++g_next_pid;
    }
    out->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

static int32_t e2e_launcher_stop(void *ctx, uint32_t process_id) {
    (void)ctx;
    pm_req_stop_t req = { .process_id = process_id };
    pm_resp_stop_t resp = {0};
    (void)process_manager_handle_stop(&req, &resp);
    return BHARAT_IPC_STATUS_OK;
}

static int32_t e2e_launcher_query(void *ctx, uint32_t process_id, uint32_t *out_state) {
    (void)ctx;
    pm_req_query_t req = { .process_id = process_id };
    pm_resp_query_t resp = {0};
    if (process_manager_handle_query(&req, &resp) == BHARAT_IPC_STATUS_OK) {
        *out_state = (resp.state == PM_STATE_STOPPING) ? PM_STATE_EXITED : resp.state;
        return BHARAT_IPC_STATUS_OK;
    }
    *out_state = PM_STATE_RUNNING;
    return BHARAT_IPC_STATUS_OK;
}

static const bh_service_launcher_ops_t g_e2e_launcher = {
    .spawn = e2e_launcher_spawn,
    .request_stop = e2e_launcher_stop,
    .query = e2e_launcher_query,
};

static const bh_servicemgr_clock_ops_t g_e2e_clock = {
    .now_ns = e2e_clock_now,
};

void test_e2e_service_orchestration(void) {
    printf("[E2E] Starting Service Orchestration Test...\n");

    process_manager_init();
    vm_manager_init();

    bh_servicemgr_dependencies_t deps = {
        .launcher = &g_e2e_launcher,
        .clock = &g_e2e_clock,
        .registry = NULL,
    };
    assert(servicemgr_init(&deps) == BHARAT_IPC_STATUS_OK);
    bh_servicemgr_t *mgr = servicemgr_get_instance();
    mgr->boot_session_id = 77ULL;

    // 1. Register and start process_manager via servicemgr
    sm_req_register_t req_reg_pm = {
        .service_name = "process_manager",
        .required_caps = 0,
        .restart_policy = SM_RESTART_POLICY_BOUNDED_RETRY,
        .critical = true,
    };
    sm_resp_register_t resp_reg_pm = {0};
    assert(servicemgr_handle_register(&req_reg_pm, &resp_reg_pm) == BHARAT_IPC_STATUS_OK);

    sm_req_start_t req_start_pm = { .service_id = resp_reg_pm.service_id };
    sm_resp_start_t resp_start_pm = {0};
    assert(servicemgr_handle_start(&req_start_pm, &resp_start_pm) == BHARAT_IPC_STATUS_OK);

    // 2. Register and start vm_manager via servicemgr
    sm_req_register_t req_reg_vm = {
        .service_name = "vm_manager",
        .required_caps = 0,
        .restart_policy = SM_RESTART_POLICY_BOUNDED_RETRY,
        .critical = false,
    };
    sm_resp_register_t resp_reg_vm = {0};
    assert(servicemgr_handle_register(&req_reg_vm, &resp_reg_vm) == BHARAT_IPC_STATUS_OK);

    sm_req_start_t req_start_vm = { .service_id = resp_reg_vm.service_id };
    sm_resp_start_t resp_start_vm = {0};
    assert(servicemgr_handle_start(&req_start_vm, &resp_start_vm) == BHARAT_IPC_STATUS_OK);

    // Verify both are STARTING
    sm_req_query_t req_q_pm = { .service_id = resp_reg_pm.service_id };
    sm_resp_query_t resp_q_pm = {0};
    assert(servicemgr_handle_query(&req_q_pm, &resp_q_pm) == BHARAT_IPC_STATUS_OK);
    assert(resp_q_pm.state == SM_STATE_STARTING);

    // Send authenticated BOUND -> READY health updates for both services
    bh_service_instance_t *inst_pm = &mgr->service_instances[0];
    bh_service_instance_t *inst_vm = &mgr->service_instances[1];

    sm_req_health_update_t hb = {
        .struct_size = sizeof(hb),
        .service_id = inst_pm->service_id,
        .process_id = inst_pm->process_id,
        .boot_session_id = mgr->boot_session_id,
        .incarnation_id = inst_pm->incarnation_id,
        .health_state = SM_HEALTH_STATE_BOUND,
        .heartbeat_sequence = 1,
    };
    sm_resp_health_update_t hb_resp = {0};
    assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_pm->process_id) == BHARAT_IPC_STATUS_OK);
    hb.health_state = SM_HEALTH_STATE_READY;
    hb.heartbeat_sequence = 2;
    assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_pm->process_id) == BHARAT_IPC_STATUS_OK);

    hb.service_id = inst_vm->service_id;
    hb.process_id = inst_vm->process_id;
    hb.incarnation_id = inst_vm->incarnation_id;
    hb.health_state = SM_HEALTH_STATE_BOUND;
    hb.heartbeat_sequence = 1;
    assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_vm->process_id) == BHARAT_IPC_STATUS_OK);
    hb.health_state = SM_HEALTH_STATE_READY;
    hb.heartbeat_sequence = 2;
    assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_vm->process_id) == BHARAT_IPC_STATUS_OK);

    assert(servicemgr_handle_query(&req_q_pm, &resp_q_pm) == BHARAT_IPC_STATUS_OK);
    assert(resp_q_pm.state == SM_STATE_RUNNING);

    // 3. Orchestrate a process creation via process_manager
    pm_req_create_t req_pm_create = { .executable_id = 999, .priority = 5 };
    pm_resp_create_t resp_pm_create = {0};
    assert(process_manager_handle_create(&req_pm_create, &resp_pm_create) == BHARAT_IPC_STATUS_OK);

    // 4. Orchestrate an address space mapping for that process via vm_manager
    vm_req_map_t req_vm_map = { .aspace_id = resp_pm_create.process_id, .vaddr = 0x400000, .size = 0x1000, .flags = 0x5 };
    vm_resp_map_t resp_vm_map = {0};
    assert(vm_manager_handle_map(&req_vm_map, &resp_vm_map) == BHARAT_IPC_STATUS_OK);
    assert(resp_vm_map.region_id == 1);

    // 5. Start the process
    pm_req_start_t req_pm_start = { .process_id = resp_pm_create.process_id };
    pm_resp_start_t resp_pm_start = {0};
    assert(process_manager_handle_start(&req_pm_start, &resp_pm_start) == BHARAT_IPC_STATUS_OK);

    // 6. Simulate a page fault for the process
    vm_req_fault_t req_vm_fault = { .aspace_id = resp_pm_create.process_id, .fault_vaddr = 0x400500, .fault_type = 0 };
    vm_resp_fault_t resp_vm_fault = {0};
    assert(vm_manager_handle_fault(&req_vm_fault, &resp_vm_fault) == BHARAT_IPC_STATUS_OK);
    assert(resp_vm_fault.action == 0);

    // 7. Stop process and unmap
    pm_req_stop_t req_pm_stop = { .process_id = resp_pm_create.process_id };
    pm_resp_stop_t resp_pm_stop = {0};
    assert(process_manager_handle_stop(&req_pm_stop, &resp_pm_stop) == BHARAT_IPC_STATUS_OK);

    vm_req_unmap_t req_vm_unmap = { .region_id = resp_vm_map.region_id };
    vm_resp_unmap_t resp_vm_unmap = {0};
    assert(vm_manager_handle_unmap(&req_vm_unmap, &resp_vm_unmap) == BHARAT_IPC_STATUS_OK);

    // 8. Simulate repeated crash and bounded backoff restart cycles for vm_manager
    uint64_t pm_hb_seq = 2;
    for (uint32_t cycle = 0; cycle < MAX_RESTART_COUNT; ++cycle) {
        g_test_now_ns += (HEARTBEAT_TIMEOUT_MS + 1000ULL) * BH_NS_PER_MS;
        hb.service_id = inst_pm->service_id;
        hb.process_id = inst_pm->process_id;
        hb.incarnation_id = inst_pm->incarnation_id;
        hb.health_state = SM_HEALTH_STATE_READY;
        hb.heartbeat_sequence = ++pm_hb_seq;
        assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_pm->process_id) == BHARAT_IPC_STATUS_OK);

        // Tick 1: Heartbeat timeout marks vm_manager FAILED and requests process stop
        servicemgr_check_health(g_test_now_ns);
        assert(inst_vm->state == SM_STATE_FAILED);
        assert(inst_vm->stop_requested == true);

        // Tick 2: Launcher query confirms PM_STATE_EXITED and enters RESTART_BACKOFF
        servicemgr_check_health(g_test_now_ns);
        assert(inst_vm->state == SM_STATE_RESTART_BACKOFF);

        g_test_now_ns = inst_vm->next_retry_ns;
        hb.service_id = inst_pm->service_id;
        hb.process_id = inst_pm->process_id;
        hb.incarnation_id = inst_pm->incarnation_id;
        hb.health_state = SM_HEALTH_STATE_READY;
        hb.heartbeat_sequence = ++pm_hb_seq;
        assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_pm->process_id) == BHARAT_IPC_STATUS_OK);

        servicemgr_check_health(g_test_now_ns);
        assert(inst_vm->state == SM_STATE_STARTING);
        assert(inst_vm->restart_count == cycle + 1U);

        hb.service_id = inst_vm->service_id;
        hb.process_id = inst_vm->process_id;
        hb.incarnation_id = inst_vm->incarnation_id;
        hb.health_state = SM_HEALTH_STATE_BOUND;
        hb.heartbeat_sequence = 1;
        assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_vm->process_id) == BHARAT_IPC_STATUS_OK);
        hb.health_state = SM_HEALTH_STATE_READY;
        hb.heartbeat_sequence = 2;
        assert(servicemgr_handle_health_update(&hb, &hb_resp, inst_vm->process_id) == BHARAT_IPC_STATUS_OK);
        assert(inst_vm->state == SM_STATE_RUNNING);
    }

    printf("[E2E] Service Orchestration Test Passed.\n");
}

int main(void) {
    test_e2e_service_orchestration();
    return 0;
}
