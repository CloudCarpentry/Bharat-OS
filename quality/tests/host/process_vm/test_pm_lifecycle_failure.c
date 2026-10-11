#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../../../core/services/process_manager/process_manager.h"
#include <bharat/uapi/ipc/status.h>

#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define EV_CURRENT      1
#define ET_EXEC         2
#define EM_X86_64       62

typedef struct {
    uint8_t   e_ident[16];
    uint16_t  e_type;
    uint16_t  e_machine;
    uint32_t  e_version;
    uint64_t  e_entry;
    uint64_t  e_phoff;
    uint64_t  e_shoff;
    uint32_t  e_flags;
    uint16_t  e_ehsize;
    uint16_t  e_phentsize;
    uint16_t  e_phnum;
    uint16_t  e_shentsize;
    uint16_t  e_shnum;
    uint16_t  e_shstrndx;
} mock_ehdr_t;

typedef struct {
    uint32_t  p_type;
    uint32_t  p_flags;
    uint64_t  p_offset;
    uint64_t  p_vaddr;
    uint64_t  p_paddr;
    uint64_t  p_filesz;
    uint64_t  p_memsz;
    uint64_t  p_align;
} mock_phdr_t;

static void setup_test_elf(uint8_t *buf, size_t buf_size) {
    memset(buf, 0, buf_size);
    mock_ehdr_t *ehdr = (mock_ehdr_t *)buf;
    ehdr->e_ident[0] = 0x7f;
    ehdr->e_ident[1] = 'E';
    ehdr->e_ident[2] = 'L';
    ehdr->e_ident[3] = 'F';
    ehdr->e_ident[4] = ELFCLASS64;
    ehdr->e_ident[5] = ELFDATA2LSB;
    ehdr->e_ident[6] = EV_CURRENT;
    ehdr->e_type = ET_EXEC;
    ehdr->e_machine = EM_X86_64;
    ehdr->e_version = EV_CURRENT;
    ehdr->e_entry = 0x2000;
    ehdr->e_phoff = sizeof(mock_ehdr_t);
    ehdr->e_ehsize = sizeof(mock_ehdr_t);
    ehdr->e_phentsize = sizeof(mock_phdr_t);
    ehdr->e_phnum = 1;

    mock_phdr_t *phdr = (mock_phdr_t *)(buf + sizeof(mock_ehdr_t));
    phdr->p_type = 1; // PT_LOAD
    phdr->p_flags = 5; // PF_X | PF_R
    phdr->p_offset = sizeof(mock_ehdr_t) + sizeof(mock_phdr_t);
    phdr->p_vaddr = 0x2000;
    phdr->p_filesz = 256;
    phdr->p_memsz = 256;
    phdr->p_align = 1; // bypass modulo check
}

// Track resources to ensure ZERO leaks on rollback
static int g_active_processes = 0;
static int g_active_spaces = 0;
static int g_active_threads = 0;

static int g_fail_terminate = 0;
static int g_fail_reap = 0;

static bharat_status_t track_create_process(void *ctx, const bh_pm_kernel_create_req_t *req, bh_pm_kernel_process_t *out_proc) {
    (void)ctx; (void)req;
    g_active_processes++;
    out_proc->pid = 5555;
    out_proc->cap = 0x1234;
    return BHARAT_STATUS_OK;
}

static bharat_status_t track_create_vm_space(void *ctx, bh_pm_kernel_process_t *proc, uint32_t memory_profile, bh_vm_kernel_space_t *out_space) {
    (void)ctx; (void)proc; (void)memory_profile;
    g_active_spaces++;
    out_space->space_id = 7777;
    return BHARAT_STATUS_OK;
}

static bharat_status_t track_realize_image(void *ctx, bh_pm_kernel_process_t *proc, bh_vm_kernel_space_t *space, const bh_user_image_plan_v1_t *plan, bh_pm_kernel_image_result_t *out_res) {
    (void)ctx; (void)proc; (void)space;
    g_active_threads++;
    out_res->main_thread_id = 8888;
    out_res->entry_point = plan->entry_point;
    return BHARAT_STATUS_OK;
}

static bharat_status_t track_start_process(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    return BHARAT_STATUS_OK;
}

static bharat_status_t track_request_terminate(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    if (g_fail_terminate) {
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }
    return BHARAT_STATUS_OK;
}

static bharat_status_t track_reap_process(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    if (g_fail_reap) {
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }
    // On failure rollback, reap is called to clean up process/thread/space
    g_active_processes = 0;
    g_active_spaces = 0;
    g_active_threads = 0;
    return BHARAT_STATUS_OK;
}

void test_lifecycle_early_failures(void) {
    process_manager_init();

    // Check Kernel ops unavailable
    assert(!bh_pm_kernel_ops_installed());
    assert(bh_pm_set_kernel_ops(NULL) == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);

    uint8_t elf_buf[1024];
    setup_test_elf(elf_buf, sizeof(elf_buf));
    bharat_status_t reg_res = bh_pm_register_executable(990011, elf_buf, sizeof(elf_buf));
    assert(reg_res == BHARAT_STATUS_OK);

    bh_pm_spawn_request_v1_t req;
    memset(&req, 0, sizeof(req));
    req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    req.struct_size = sizeof(req);
    req.executable_handle = 990011;
    strcpy(req.process_name, "test_prog");

    // Spawn fails when kernel ops not installed
    bh_pm_spawn_response_v1_t unsupported_resp;
    int status = bh_pm_handle_spawn_v1(&req, &unsupported_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
    assert(bh_pm_get_active_count() == 0);

    // Install mock kernel ops
    bh_pm_kernel_ops_t ops = {
        .ctx = NULL,
        .create_process = track_create_process,
        .create_vm_space = track_create_vm_space,
        .realize_image = track_realize_image,
        .start_process = track_start_process,
        .request_terminate = track_request_terminate,
        .reap_process = track_reap_process
    };
    assert(bh_pm_set_kernel_ops(&ops) == BHARAT_IPC_STATUS_OK);
    assert(bh_pm_kernel_ops_installed());

    // Test failure injection at each early stage (1-5)
    for (int fail_stage = 1; fail_stage <= 5; fail_stage++) {
        bh_pm_set_failure_injection(fail_stage);

        bh_pm_spawn_response_v1_t resp;
        status = bh_pm_handle_spawn_v1(&req, &resp);
        assert(status != BHARAT_IPC_STATUS_OK);
        assert(resp.status != BHARAT_IPC_STATUS_OK);

        // Assert handle and active process/space/thread count is rolled back to ZERO
        assert(bh_pm_get_active_count() == 0);
        assert(g_active_processes == 0);
        assert(g_active_spaces == 0);
        assert(g_active_threads == 0);
    }
    bh_pm_set_failure_injection(0);
}

void test_lifecycle_late_failures(void) {
    // Re-initialize completely
    process_manager_init();

    // Ensure ops are still set for late failures
    bh_pm_kernel_ops_t ops = {
        .ctx = NULL,
        .create_process = track_create_process,
        .create_vm_space = track_create_vm_space,
        .realize_image = track_realize_image,
        .start_process = track_start_process,
        .request_terminate = track_request_terminate,
        .reap_process = track_reap_process
    };
    bh_pm_set_kernel_ops(&ops);

    uint8_t elf_buf[1024];
    setup_test_elf(elf_buf, sizeof(elf_buf));
    bh_pm_register_executable(990011, elf_buf, sizeof(elf_buf));

    bh_pm_spawn_request_v1_t req;
    memset(&req, 0, sizeof(req));
    req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    req.struct_size = sizeof(req);
    req.executable_handle = 990011; // Assume already registered
    strcpy(req.process_name, "test_prog_late");

    // Spawn a successful process first
    bh_pm_spawn_response_v1_t resp;
    int status = bh_pm_handle_spawn_v1(&req, &resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp.process_handle != 0);
    assert(bh_pm_get_active_count() == 1);

    bh_pm_handle_t proc_handle = resp.process_handle;

    // Fail termination
    g_fail_terminate = 1;
    bh_pm_terminate_request_v1_t t_req;
    memset(&t_req, 0, sizeof(t_req));
    t_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    t_req.struct_size = sizeof(t_req);
    t_req.process_handle = proc_handle;

    bh_pm_terminate_response_v1_t t_resp;
    status = bh_pm_handle_terminate_v1(&t_req, &t_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INTERNAL);
    g_fail_terminate = 0;

    // Success termination
    status = bh_pm_handle_terminate_v1(&t_req, &t_resp);
    assert(status == BHARAT_IPC_STATUS_OK);

    // Simulate async kernel notification of exit
    bh_pm_notify_exit_v1(resp.kernel_process_id, 42, 0);

    // Fail reap
    g_fail_reap = 1;
    bh_pm_reap_request_v1_t r_req;
    memset(&r_req, 0, sizeof(r_req));
    r_req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    r_req.struct_size = sizeof(r_req);
    r_req.process_handle = proc_handle;

    bh_pm_reap_response_v1_t r_resp;
    status = bh_pm_handle_reap_v1(&r_req, &r_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INTERNAL);
    g_fail_reap = 0;

    // Success reap
    status = bh_pm_handle_reap_v1(&r_req, &r_resp);
    assert(status == BHARAT_IPC_STATUS_OK);

    // Verify handle table is fully cleared (0 active processes remaining)
    assert(bh_pm_get_active_count() == 0);
    assert(g_active_processes == 0);
    assert(g_active_spaces == 0);
    assert(g_active_threads == 0);

    // Ensure we can successfully repeat spawns after cleanup
    status = bh_pm_handle_spawn_v1(&req, &resp);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(bh_pm_get_active_count() == 1);

    // Quick reap again for the second spawn
    t_req.process_handle = resp.process_handle;
    bh_pm_handle_terminate_v1(&t_req, &t_resp);
    bh_pm_notify_exit_v1(resp.kernel_process_id, 0, 0);
    r_req.process_handle = resp.process_handle;
    bh_pm_handle_reap_v1(&r_req, &r_resp);
    assert(bh_pm_get_active_count() == 0);
}

int main(void) {
    test_lifecycle_early_failures();
    test_lifecycle_late_failures();
    printf("test_pm_lifecycle_failure passed!\n");
    return 0;
}
