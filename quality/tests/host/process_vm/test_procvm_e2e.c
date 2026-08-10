#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../../../core/services/process_manager/process_manager.h"
#include <bharat/uapi/ipc/status.h>

void test_procvm_e2e_lifecycle(void) {
    process_manager_init();

    // Register a valid mock ELF binary programmatically
    uint8_t dummy_elf[1024];
    memset(dummy_elf, 0, sizeof(dummy_elf));
    dummy_elf[0] = 0x7f; dummy_elf[1] = 'E'; dummy_elf[2] = 'L'; dummy_elf[3] = 'F';
    dummy_elf[4] = 2; dummy_elf[5] = 1; dummy_elf[6] = 1;
    dummy_elf[16] = 2; dummy_elf[17] = 0; // ET_EXEC
    dummy_elf[18] = 62; dummy_elf[19] = 0; // EM_X86_64
    dummy_elf[20] = 1; // e_version
    dummy_elf[32] = 64; // e_phoff
    dummy_elf[54] = 56; // e_phentsize
    dummy_elf[56] = 1; // e_phnum
    dummy_elf[24] = 0x00; dummy_elf[25] = 0x10; // e_entry = 0x1000

    // Segment header
    dummy_elf[64] = 1; // PT_LOAD
    dummy_elf[68] = 5; // PF_X | PF_R
    dummy_elf[72] = 120; // p_offset
    dummy_elf[80] = 0x00; dummy_elf[81] = 0x10; // p_vaddr
    dummy_elf[96] = 100; // p_filesz
    dummy_elf[104] = 100; // p_memsz

    int reg_res = bh_pm_register_executable(0xABCD, dummy_elf, sizeof(dummy_elf));
    assert(reg_res == 0);

    // Spawn Request
    bh_pm_spawn_request_v1_t spawn_req;
    memset(&spawn_req, 0, sizeof(spawn_req));
    spawn_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    spawn_req.struct_size = sizeof(spawn_req);
    spawn_req.executable_handle = 0xABCD;
    strcpy(spawn_req.process_name, "e2e_test");
    spawn_req.priority = 15;

    bh_pm_spawn_response_v1_t spawn_resp;
    int status = bh_pm_handle_spawn_v1(&spawn_req, &spawn_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(spawn_resp.process_handle != 0);

    bh_pm_handle_t proc_handle = spawn_resp.process_handle;

    // Simulate process running and exit code 42 notification
    bh_pm_notify_exit_v1(spawn_resp.kernel_process_id, 42, 0);

    // Wait Request
    bh_pm_wait_request_v1_t wait_req;
    memset(&wait_req, 0, sizeof(wait_req));
    wait_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    wait_req.struct_size = sizeof(wait_req);
    wait_req.process_handle = proc_handle;
    wait_req.wait_flags = BH_PM_WAIT_NONBLOCK;

    bh_pm_wait_response_v1_t wait_resp;
    status = bh_pm_handle_wait_v1(&wait_req, &wait_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(wait_resp.exit_code == 42);

    // Reap Request
    bh_pm_reap_request_v1_t reap_req;
    memset(&reap_req, 0, sizeof(reap_req));
    reap_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    reap_req.struct_size = sizeof(reap_req);
    reap_req.process_handle = proc_handle;

    bh_pm_reap_response_v1_t reap_resp;
    status = bh_pm_handle_reap_v1(&reap_req, &reap_resp);
    assert(status == BHARAT_IPC_STATUS_OK);

    // Verify handle table is fully cleared (0 active processes remaining)
    assert(bh_pm_get_active_count() == 0);

    printf("test_procvm_e2e_lifecycle passed successfully!\n");
}

int main(void) {
    test_procvm_e2e_lifecycle();
    return 0;
}
