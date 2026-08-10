#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../../../core/services/process_manager/process_manager.h"
#include <bharat/uapi/ipc/status.h>

void test_pm_wait_reap_lifecycle(void) {
    process_manager_init();

    // Register dummy executable
    uint8_t dummy_elf[1024];
    // setup minimalist valid ELF header
    memset(dummy_elf, 0, sizeof(dummy_elf));
    dummy_elf[0] = 0x7f; dummy_elf[1] = 'E'; dummy_elf[2] = 'L'; dummy_elf[3] = 'F';
    dummy_elf[4] = 2; dummy_elf[5] = 1; dummy_elf[6] = 1;
    // e_type = ET_EXEC
    dummy_elf[16] = 2; dummy_elf[17] = 0;
    // e_machine = EM_X86_64
    dummy_elf[18] = 62; dummy_elf[19] = 0;
    // e_version = 1
    dummy_elf[20] = 1;
    // e_phoff = 64
    dummy_elf[32] = 64;
    // e_phentsize = 56
    dummy_elf[54] = 56;
    // e_phnum = 1
    dummy_elf[56] = 1;
    // e_entry = 0x1000
    dummy_elf[24] = 0x00; dummy_elf[25] = 0x10;

    // segment
    // p_type = PT_LOAD (1)
    dummy_elf[64] = 1;
    // p_flags = PF_X | PF_R (5)
    dummy_elf[68] = 5;
    // p_offset = 120
    dummy_elf[72] = 120;
    // p_vaddr = 0x1000
    dummy_elf[80] = 0x00; dummy_elf[81] = 0x10;
    // p_filesz = 100
    dummy_elf[96] = 100;
    // p_memsz = 100
    dummy_elf[104] = 100;

    bh_pm_register_executable(1122, dummy_elf, sizeof(dummy_elf));

    bh_pm_spawn_request_v1_t req;
    memset(&req, 0, sizeof(req));
    req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    req.struct_size = sizeof(req);
    req.executable_handle = 1122;
    strcpy(req.process_name, "wait_prog");

    bh_pm_spawn_response_v1_t resp;
    int status = bh_pm_handle_spawn_v1(&req, &resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp.process_handle != 0);

    bh_pm_handle_t proc_handle = resp.process_handle;

    // Wait nonblock check - should return BUSY since process is still running
    bh_pm_wait_request_v1_t w_req;
    memset(&w_req, 0, sizeof(w_req));
    w_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    w_req.struct_size = sizeof(w_req);
    w_req.process_handle = proc_handle;
    w_req.wait_flags = BH_PM_WAIT_NONBLOCK;

    bh_pm_wait_response_v1_t w_resp;
    status = bh_pm_handle_wait_v1(&w_req, &w_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_BUSY);

    // Call terminate
    bh_pm_terminate_request_v1_t t_req;
    memset(&t_req, 0, sizeof(t_req));
    t_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    t_req.struct_size = sizeof(t_req);
    t_req.process_handle = proc_handle;

    bh_pm_terminate_response_v1_t t_resp;
    status = bh_pm_handle_terminate_v1(&t_req, &t_resp);
    assert(status == BHARAT_IPC_STATUS_OK);

    // Simulate async kernel notification of exit
    bh_pm_notify_exit_v1(resp.kernel_process_id, 42, 0);

    // Query state - should show EXITED (Zombie)
    bh_pm_query_request_v1_t q_req;
    memset(&q_req, 0, sizeof(q_req));
    q_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    q_req.struct_size = sizeof(q_req);
    q_req.process_handle = proc_handle;

    bh_pm_query_response_v1_t q_resp;
    status = bh_pm_handle_query_v1(&q_req, &q_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(q_resp.state == BH_PM_STATE_EXITED_V1);
    assert(q_resp.exit_code == 42);

    // Wait now should return 42 immediately
    status = bh_pm_handle_wait_v1(&w_req, &w_resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(w_resp.exit_code == 42);

    // Reap process
    bh_pm_reap_request_v1_t r_req;
    memset(&r_req, 0, sizeof(r_req));
    r_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    r_req.struct_size = sizeof(r_req);
    r_req.process_handle = proc_handle;

    bh_pm_reap_response_v1_t r_resp;
    status = bh_pm_handle_reap_v1(&r_req, &r_resp);
    assert(status == BHARAT_IPC_STATUS_OK);

    // Second reap should fail (Stale handle)
    status = bh_pm_handle_reap_v1(&r_req, &r_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_NOT_FOUND);

    // Query on reaped handle should fail
    status = bh_pm_handle_query_v1(&q_req, &q_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_NOT_FOUND);

    printf("test_pm_wait_reap_lifecycle passed!\n");
}

int main(void) {
    test_pm_wait_reap_lifecycle();
    return 0;
}
