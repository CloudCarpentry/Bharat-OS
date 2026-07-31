#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../core/services/process_manager/process_manager.h"
#include <bharat/uapi/ipc/status.h>

// Metrics tracking to prove zero resource leaks
static int g_processes = 0;
static int g_spaces = 0;
static int g_threads = 0;

static int32_t stress_create_process(void *ctx, const bh_pm_kernel_create_req_t *req, bh_pm_kernel_process_t *out_proc) {
    (void)ctx; (void)req;
    g_processes++;
    out_proc->pid = 12000 + g_processes;
    return 0;
}

static int32_t stress_create_vm_space(void *ctx, bh_pm_kernel_process_t *proc, uint32_t memory_profile, bh_vm_kernel_space_t *out_space) {
    (void)ctx; (void)proc; (void)memory_profile;
    g_spaces++;
    out_space->space_id = 14000 + g_spaces;
    return 0;
}

static int32_t stress_realize_image(void *ctx, bh_pm_kernel_process_t *proc, bh_vm_kernel_space_t *space, const bh_user_image_plan_v1_t *plan, bh_pm_kernel_image_result_t *out_res) {
    (void)ctx; (void)proc; (void)space;
    g_threads++;
    out_res->main_thread_id = 16000 + g_threads;
    out_res->entry_point = plan->entry_point;
    return 0;
}

static int32_t stress_start_process(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    return 0;
}

static int32_t stress_request_terminate(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    return 0;
}

static int32_t stress_reap_process(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    g_processes--;
    g_spaces--;
    g_threads--;
    return 0;
}

void test_procvm_1000_cycles_stress(void) {
    process_manager_init();

    bh_pm_kernel_ops_t pm_ops = {
        .ctx = NULL,
        .create_process = stress_create_process,
        .create_vm_space = stress_create_vm_space,
        .realize_image = stress_realize_image,
        .start_process = stress_start_process,
        .request_terminate = stress_request_terminate,
        .reap_process = stress_reap_process
    };
    bh_pm_set_kernel_ops(&pm_ops);

    // Register dummy executable
    uint8_t dummy_elf[1024];
    memset(dummy_elf, 0, sizeof(dummy_elf));
    dummy_elf[0] = 0x7f; dummy_elf[1] = 'E'; dummy_elf[2] = 'L'; dummy_elf[3] = 'F';
    dummy_elf[4] = 2; dummy_elf[5] = 1; dummy_elf[6] = 1;
    dummy_elf[16] = 2; dummy_elf[17] = 0;
    dummy_elf[18] = 62; dummy_elf[19] = 0;
    dummy_elf[20] = 1;
    dummy_elf[32] = 64;
    dummy_elf[54] = 56;
    dummy_elf[56] = 1;
    dummy_elf[24] = 0x00; dummy_elf[25] = 0x10;

    dummy_elf[64] = 1;
    dummy_elf[68] = 5;
    dummy_elf[72] = 120;
    dummy_elf[80] = 0x00; dummy_elf[81] = 0x10;
    dummy_elf[96] = 100;
    dummy_elf[104] = 100;

    bh_pm_register_executable(0x777, dummy_elf, sizeof(dummy_elf));

    // Run 1,000 cycles
    for (int cycle = 1; cycle <= 1000; cycle++) {
        // Deterministic failure injection test occasionally
        if (cycle % 50 == 0) {
            bh_pm_set_failure_injection(1 + (cycle % 5));
            bh_pm_spawn_request_v1_t s_req;
            memset(&s_req, 0, sizeof(s_req));
            s_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
            s_req.struct_size = sizeof(s_req);
            s_req.executable_handle = 0x777;
            strcpy(s_req.process_name, "fail_test");

            bh_pm_spawn_response_v1_t s_resp;
            int status = bh_pm_handle_spawn_v1(&s_req, &s_resp);
            assert(status != BHARAT_IPC_STATUS_OK);

            // Assert everything is back to original baseline on failure rollback
            assert(bh_pm_get_active_count() == 0);
            assert(g_processes == 0);
            assert(g_spaces == 0);
            assert(g_threads == 0);
        }

        // Standard successful cycle
        bh_pm_set_failure_injection(0);

        bh_pm_spawn_request_v1_t s_req;
        memset(&s_req, 0, sizeof(s_req));
        s_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
        s_req.struct_size = sizeof(s_req);
        s_req.executable_handle = 0x777;
        strcpy(s_req.process_name, "stress_prog");

        bh_pm_spawn_response_v1_t s_resp;
        int status = bh_pm_handle_spawn_v1(&s_req, &s_resp);
        assert(status == BHARAT_IPC_STATUS_OK);
        assert(s_resp.process_handle != 0);

        // Notify exit asynchronously
        bh_pm_notify_exit_v1(s_resp.kernel_process_id, 42, 0);

        // Wait
        bh_pm_wait_request_v1_t w_req;
        memset(&w_req, 0, sizeof(w_req));
        w_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
        w_req.struct_size = sizeof(w_req);
        w_req.process_handle = s_resp.process_handle;
        w_req.wait_flags = BH_PM_WAIT_NONBLOCK;

        bh_pm_wait_response_v1_t w_resp;
        status = bh_pm_handle_wait_v1(&w_req, &w_resp);
        assert(status == BHARAT_IPC_STATUS_OK);
        assert(w_resp.exit_code == 42);

        // Reap
        bh_pm_reap_request_v1_t r_req;
        memset(&r_req, 0, sizeof(r_req));
        r_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
        r_req.struct_size = sizeof(r_req);
        r_req.process_handle = s_resp.process_handle;

        bh_pm_reap_response_v1_t r_resp;
        status = bh_pm_handle_reap_v1(&r_req, &r_resp);
        assert(status == BHARAT_IPC_STATUS_OK);

        // Verify baseline metrics after EVERY cycle
        assert(bh_pm_get_active_count() == 0);
        assert(g_processes == 0);
        assert(g_spaces == 0);
        assert(g_threads == 0);
    }

    printf("test_procvm_1000_cycles_stress completed with absolutely ZERO leaks!\n");
}

int main(void) {
    test_procvm_1000_cycles_stress();
    return 0;
}
