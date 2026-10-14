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

static bharat_status_t track_create_process(void *ctx, const bh_pm_kernel_create_req_t *req, bh_pm_kernel_process_t *out_proc) {
    (void)ctx; (void)req;
    g_active_processes++;
    out_proc->pid = 5555;
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
    return BHARAT_STATUS_OK;
}

static bharat_status_t track_reap_process(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    // On failure rollback, reap is called to clean up process/thread/space
    g_active_processes = 0;
    g_active_spaces = 0;
    g_active_threads = 0;
    return BHARAT_STATUS_OK;
}

void test_spawn_rollback_failures(void) {
    process_manager_init();
    assert(!bh_pm_kernel_ops_installed());
    assert(bh_pm_set_kernel_ops(NULL) == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);

    uint8_t elf_buf[1024];
    setup_test_elf(elf_buf, sizeof(elf_buf));
    bharat_status_t reg_res = bh_pm_register_executable(990011, elf_buf, sizeof(elf_buf));
    assert(reg_res == BHARAT_STATUS_OK);
    assert(bh_pm_register_executable(990011, elf_buf, sizeof(elf_buf)) == BHARAT_STATUS_ERR_ALREADY_EXISTS);
    assert(bh_pm_register_executable(0, elf_buf, sizeof(elf_buf)) == BHARAT_STATUS_ERR_INVALID_ARG);
    assert(bh_pm_register_executable(990012, NULL, sizeof(elf_buf)) == BHARAT_STATUS_ERR_INVALID_ARG);

    bh_pm_spawn_request_v1_t req;
    memset(&req, 0, sizeof(req));
    req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    req.struct_size = sizeof(req);
    req.executable_handle = 990011;
    strcpy(req.process_name, "test_prog");

    bh_pm_spawn_response_v1_t invalid_resp;
    req.reserved = 1;
    int status = bh_pm_handle_spawn_v1(&req, &invalid_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);
    assert(bh_pm_get_active_count() == 0);
    req.reserved = 0;

    /* An uninstalled adapter must not manufacture process identifiers. */
    bh_pm_spawn_response_v1_t unsupported_resp;
    status = bh_pm_handle_spawn_v1(&req, &unsupported_resp);
    assert(status == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
    assert(unsupported_resp.status == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
    assert(unsupported_resp.kernel_process_id == 0);
    assert(bh_pm_get_active_count() == 0);

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

    // Test failure injection at each stage
    for (int fail_stage = 1; fail_stage <= 5; fail_stage++) {
        bh_pm_set_failure_injection(fail_stage);

        bh_pm_spawn_response_v1_t resp;
        status = bh_pm_handle_spawn_v1(&req, &resp);
        assert(status != BHARAT_IPC_STATUS_OK);
        assert(resp.status != BHARAT_IPC_STATUS_OK);

        // Assert handle and active process/space/thread count is completely rolled back to ZERO
        assert(bh_pm_get_active_count() == 0);
        assert(g_active_processes == 0);
        assert(g_active_spaces == 0);
        assert(g_active_threads == 0);
    }

    // Now test a successful spawn
    bh_pm_set_failure_injection(0);
    bh_pm_spawn_response_v1_t resp;
    status = bh_pm_handle_spawn_v1(&req, &resp);
    printf("Debug status: %d, resp.status: %d\n", status, resp.status);
    fflush(stdout);
    assert(status == BHARAT_IPC_STATUS_OK);
    assert(resp.status == BHARAT_IPC_STATUS_OK);
    assert(resp.process_handle != 0);
    assert(bh_pm_get_active_count() == 1);

    bh_pm_query_request_v1_t bad_query = {
        .abi_version = BH_PM_INTERFACE_VERSION_V1 + 1,
        .struct_size = sizeof(bad_query),
        .process_handle = resp.process_handle,
    };
    bh_pm_query_response_v1_t query_resp;
    assert(bh_pm_handle_query_v1(&bad_query, &query_resp) ==
           BHARAT_IPC_STATUS_ERR_INVALID);

    bh_pm_wait_request_v1_t bad_wait = {
        .abi_version = BH_PM_INTERFACE_VERSION_V1,
        .struct_size = sizeof(bad_wait),
        .process_handle = resp.process_handle,
        .wait_flags = UINT32_MAX,
    };
    bh_pm_wait_response_v1_t wait_resp;
    assert(bh_pm_handle_wait_v1(&bad_wait, &wait_resp) ==
           BHARAT_IPC_STATUS_ERR_INVALID);

    bad_wait.wait_flags = 0;
    assert(bh_pm_handle_wait_v1(&bad_wait, &wait_resp) ==
           BHARAT_IPC_STATUS_ERR_UNSUPPORTED);

    printf("test_spawn_rollback_failures passed!\n");
}

static uint8_t s_boot_elf[1024];

static bharat_status_t mock_acquire_boot_module(void *ctx, const char *module_name, const uint8_t **out_elf_buf, size_t *out_elf_size) {
    (void)ctx;
    if (strcmp(module_name, "test_boot_mod") == 0) {
        setup_test_elf(s_boot_elf, sizeof(s_boot_elf));
        *out_elf_buf = s_boot_elf;
        *out_elf_size = sizeof(s_boot_elf);
        return BHARAT_IPC_STATUS_OK;
    }
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

void test_register_executable_v1(void) {
    process_manager_init();

    bh_pm_register_executable_request_v1_t req;
    bh_pm_register_executable_response_v1_t resp;

    // NULL checks
    assert(bh_pm_handle_register_executable_v1(NULL, &resp) == BHARAT_IPC_STATUS_ERR_INVALID);
    assert(bh_pm_handle_register_executable_v1(&req, NULL) == BHARAT_IPC_STATUS_ERR_INVALID);

    // Invalid ABI / size / handle / module
    memset(&req, 0, sizeof(req));
    req.abi_version = BH_PM_INTERFACE_VERSION_V1 + 1;
    req.struct_size = sizeof(req);
    req.executable_handle = 0x1234;
    strncpy(req.module_name, "test_boot_mod", sizeof(req.module_name));
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_ERR_INVALID);

    req.abi_version = BH_PM_INTERFACE_VERSION_V1;
    req.struct_size = sizeof(req) - 1;
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_ERR_INVALID);

    req.struct_size = sizeof(req);
    req.executable_handle = 0;
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_ERR_INVALID);

    req.executable_handle = 0x1234;
    req.module_name[0] = '\0';
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_ERR_INVALID);

    strncpy(req.module_name, "test_boot_mod", sizeof(req.module_name));

    // Without acquire_boot_module installed, must fail with UNSUPPORTED
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
    assert(resp.status == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);

    // Install ops with mock_acquire_boot_module
    bh_pm_kernel_ops_t ops = {
        .ctx = NULL,
        .acquire_boot_module = mock_acquire_boot_module,
        .create_process = track_create_process,
        .create_vm_space = track_create_vm_space,
        .realize_image = track_realize_image,
        .start_process = track_start_process,
        .request_terminate = track_request_terminate,
        .reap_process = track_reap_process
    };
    assert(bh_pm_set_kernel_ops(&ops) == BHARAT_IPC_STATUS_OK);

    // Non-existent module
    strncpy(req.module_name, "non_existent", sizeof(req.module_name));
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_ERR_NOT_FOUND);
    assert(resp.status == BHARAT_IPC_STATUS_ERR_NOT_FOUND);

    // Existing module
    strncpy(req.module_name, "test_boot_mod", sizeof(req.module_name));
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_OK);
    assert(resp.status == BHARAT_IPC_STATUS_OK);
    assert(resp.image_size == sizeof(s_boot_elf));
    assert(resp.executable_handle == 0x1234);

    // Idempotent re-registration
    memset(&resp, 0, sizeof(resp));
    assert(bh_pm_handle_register_executable_v1(&req, &resp) == BHARAT_IPC_STATUS_OK);
    assert(resp.status == BHARAT_IPC_STATUS_OK);
    assert(resp.image_size == sizeof(s_boot_elf));

    // Now spawn with registered executable handle 0x1234
    bh_pm_spawn_request_v1_t spawn_req = {
        .abi_version = BH_PM_INTERFACE_VERSION_V1,
        .struct_size = sizeof(spawn_req),
        .executable_handle = 0x1234,
    };
    strncpy(spawn_req.process_name, "boot_child", sizeof(spawn_req.process_name));
    bh_pm_spawn_response_v1_t spawn_resp;
    assert(bh_pm_handle_spawn_v1(&spawn_req, &spawn_resp) == BHARAT_IPC_STATUS_OK);
    assert(spawn_resp.status == BHARAT_IPC_STATUS_OK);
    assert(spawn_resp.process_handle != 0);

    printf("test_register_executable_v1 passed!\n");
}

int main(void) {
    test_spawn_rollback_failures();
    test_register_executable_v1();
    return 0;
}

