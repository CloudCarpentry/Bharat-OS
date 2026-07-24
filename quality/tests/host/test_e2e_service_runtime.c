#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "../../core/services/process_manager/process_manager.h"
#include "../../core/services/vm_manager/vm_manager.h"
#include "../../core/services/servicemgr/servicemgr.h"
#include <bharat/uapi/ipc/status.h>

void test_e2e_service_orchestration(void) {
    printf("[E2E] Starting Service Orchestration Test...\n");

    // Initialize services
    process_manager_init();
    vm_manager_init();
    servicemgr_init();

    // 1. Register and start process_manager via servicemgr
    sm_req_register_t req_reg_pm = { .service_name = "process_manager", .required_caps = 0 };
    sm_resp_register_t resp_reg_pm;
    assert(servicemgr_handle_register(&req_reg_pm, &resp_reg_pm) == BHARAT_IPC_STATUS_OK);

    sm_req_start_t req_start_pm = { .service_id = resp_reg_pm.service_id };
    sm_resp_start_t resp_start_pm;
    assert(servicemgr_handle_start(&req_start_pm, &resp_start_pm) == BHARAT_IPC_STATUS_OK);

    // 2. Register and start vm_manager via servicemgr
    sm_req_register_t req_reg_vm = { .service_name = "vm_manager", .required_caps = 0 };
    sm_resp_register_t resp_reg_vm;
    assert(servicemgr_handle_register(&req_reg_vm, &resp_reg_vm) == BHARAT_IPC_STATUS_OK);

    sm_req_start_t req_start_vm = { .service_id = resp_reg_vm.service_id };
    sm_resp_start_t resp_start_vm;
    assert(servicemgr_handle_start(&req_start_vm, &resp_start_vm) == BHARAT_IPC_STATUS_OK);

    // Verify both are STARTING/RUNNING
    sm_req_query_t req_q_pm = { .service_id = resp_reg_pm.service_id };
    sm_resp_query_t resp_q_pm;
    assert(servicemgr_handle_query(&req_q_pm, &resp_q_pm) == BHARAT_IPC_STATUS_OK);
    assert(resp_q_pm.state == SM_STATE_STARTING);

    // Send heartbeats to move to RUNNING
    sm_req_heartbeat_t req_hb_pm = { .service_id = resp_reg_pm.service_id, .timestamp = 100 };
    sm_resp_heartbeat_t resp_hb_pm;
    assert(servicemgr_handle_heartbeat(&req_hb_pm, &resp_hb_pm) == BHARAT_IPC_STATUS_OK);

    sm_req_heartbeat_t req_hb_vm = { .service_id = resp_reg_vm.service_id, .timestamp = 100 };
    sm_resp_heartbeat_t resp_hb_vm;
    assert(servicemgr_handle_heartbeat(&req_hb_vm, &resp_hb_vm) == BHARAT_IPC_STATUS_OK);

    assert(servicemgr_handle_query(&req_q_pm, &resp_q_pm) == BHARAT_IPC_STATUS_OK);
    assert(resp_q_pm.state == SM_STATE_RUNNING);

    // 3. Orchestrate a process creation via process_manager
    pm_req_create_t req_pm_create = { .executable_id = 999, .priority = 5 };
    pm_resp_create_t resp_pm_create;
    assert(process_manager_handle_create(&req_pm_create, &resp_pm_create) == BHARAT_IPC_STATUS_OK);
    assert(resp_pm_create.process_id == 1);

    // 4. Orchestrate an address space mapping for that process via vm_manager
    vm_req_map_t req_vm_map = { .aspace_id = resp_pm_create.process_id, .vaddr = 0x400000, .size = 0x1000, .flags = 0x5 };
    vm_resp_map_t resp_vm_map;
    assert(vm_manager_handle_map(&req_vm_map, &resp_vm_map) == BHARAT_IPC_STATUS_OK);
    assert(resp_vm_map.region_id == 1);

    // 5. Start the process
    pm_req_start_t req_pm_start = { .process_id = resp_pm_create.process_id };
    pm_resp_start_t resp_pm_start;
    assert(process_manager_handle_start(&req_pm_start, &resp_pm_start) == BHARAT_IPC_STATUS_OK);

    // 6. Simulate a page fault for the process
    vm_req_fault_t req_vm_fault = { .aspace_id = resp_pm_create.process_id, .fault_vaddr = 0x400500, .fault_type = 0 };
    vm_resp_fault_t resp_vm_fault;
    assert(vm_manager_handle_fault(&req_vm_fault, &resp_vm_fault) == BHARAT_IPC_STATUS_OK);
    assert(resp_vm_fault.action == 0); // successfully resolved

    // 7. Stop process and unmap
    pm_req_stop_t req_pm_stop = { .process_id = resp_pm_create.process_id };
    pm_resp_stop_t resp_pm_stop;
    assert(process_manager_handle_stop(&req_pm_stop, &resp_pm_stop) == BHARAT_IPC_STATUS_OK);

    vm_req_unmap_t req_vm_unmap = { .region_id = resp_vm_map.region_id };
    vm_resp_unmap_t resp_vm_unmap;
    assert(vm_manager_handle_unmap(&req_vm_unmap, &resp_vm_unmap) == BHARAT_IPC_STATUS_OK);

    // 8. Simulate process_manager crash and restart
    servicemgr_check_health(10000); // Exceed heartbeat timeout

    assert(servicemgr_handle_query(&req_q_pm, &resp_q_pm) == BHARAT_IPC_STATUS_OK);
    assert(resp_q_pm.state == SM_STATE_STARTING); // Automatically restarted
    assert(resp_q_pm.restart_count == 1);

    printf("[E2E] Service Orchestration Test Passed.\n");
}

int main(void) {
    test_e2e_service_orchestration();
    return 0;
}
