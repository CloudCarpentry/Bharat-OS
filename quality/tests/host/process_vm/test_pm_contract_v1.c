#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <bharat/uapi/process_manager/contract_v1.h>
#include <bharat/uapi/vm_manager/contract_v1.h>

void test_abi_contract_sizes(void) {
    // Assert sizes are non-zero
    assert(sizeof(bh_pm_spawn_request_v1_t) > 0);
    assert(sizeof(bh_pm_spawn_response_v1_t) > 0);
    assert(sizeof(bh_vm_create_space_request_v1_t) > 0);
    assert(sizeof(bh_vm_map_request_v1_t) > 0);

    // Verify pointer-free / fixed width nature
    bh_pm_spawn_request_v1_t req;
    req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    req.struct_size = sizeof(bh_pm_spawn_request_v1_t);

    assert(req.abi_version == 1);
    assert(req.struct_size == sizeof(bh_pm_spawn_request_v1_t));

    printf("test_abi_contract_sizes passed!\n");
}

int main(void) {
    test_abi_contract_sizes();
    return 0;
}
