#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../services/vm_manager/vm_manager.h"
#include <bharat/uapi/ipc/status.h>

void test_vm_lifecycle(void) {
    vm_manager_init();

    vm_req_map_t req_map = { .aspace_id = 10, .vaddr = 0x1000, .size = 0x1000, .flags = 0x3 };
    vm_resp_map_t resp_map;
    int32_t status = vm_manager_handle_map(&req_map, &resp_map);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp_map.region_id == 1);

    vm_req_query_t req_query = { .region_id = 1 };
    vm_resp_query_t resp_query;
    status = vm_manager_handle_query(&req_query, &resp_query);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp_query.state == VM_REGION_DECLARED);

    vm_req_protect_t req_protect = { .region_id = 1, .new_flags = 0x1 };
    vm_resp_protect_t resp_protect;
    status = vm_manager_handle_protect(&req_protect, &resp_protect);
    assert(status == BHARAT_IPC_STATUS_OK);

    status = vm_manager_handle_query(&req_query, &resp_query);
    assert(resp_query.state == VM_REGION_VALIDATED);

    vm_req_fault_t req_fault = { .aspace_id = 10, .fault_vaddr = 0x1500, .fault_type = 0 };
    vm_resp_fault_t resp_fault;
    status = vm_manager_handle_fault(&req_fault, &resp_fault);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp_fault.action == 0); // resolved

    status = vm_manager_handle_query(&req_query, &resp_query);
    assert(resp_query.state == VM_REGION_ACTIVE);

    vm_req_unmap_t req_unmap = { .region_id = 1 };
    vm_resp_unmap_t resp_unmap;
    status = vm_manager_handle_unmap(&req_unmap, &resp_unmap);
    assert(status == BHARAT_IPC_STATUS_OK);

    status = vm_manager_handle_query(&req_query, &resp_query);
    assert(resp_query.state == VM_REGION_REVOKED);

    printf("test_vm_manager passed\n");
}

int main(void) {
    test_vm_lifecycle();
    return 0;
}
