#include "process_manager.h"
#include <stddef.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>

// Handle library includes
#include <handle_table.h>

// Custom basic mem/string implementations to ensure freestanding compilation
static void *local_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

static void *local_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

static size_t local_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static char *local_strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

static process_entry_t process_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

// v1 Handle table and arrays
static bh_user_handle_table_t g_pm_handle_table;
static bh_user_handle_slot_t g_pm_handle_slots[MAX_PROCESSES];
static bh_pm_process_v1_t g_pm_processes[MAX_PROCESSES];

static bh_pm_executable_image_t g_executables[MAX_EXECUTABLES];
static int g_fail_stage = 0;

// Default Kernel Ops implementation (no-op stubs)
static int32_t default_create_process(void *ctx, const bh_pm_kernel_create_req_t *req, bh_pm_kernel_process_t *out_proc) {
    (void)ctx; (void)req;
    out_proc->pid = 4200;
    return 0;
}

static int32_t default_create_vm_space(void *ctx, bh_pm_kernel_process_t *proc, uint32_t memory_profile, bh_vm_kernel_space_t *out_space) {
    (void)ctx; (void)proc; (void)memory_profile;
    out_space->space_id = 9900;
    return 0;
}

static int32_t default_realize_image(void *ctx, bh_pm_kernel_process_t *proc, bh_vm_kernel_space_t *space, const bh_user_image_plan_v1_t *plan, bh_pm_kernel_image_result_t *out_res) {
    (void)ctx; (void)proc; (void)space;
    out_res->main_thread_id = 1100;
    out_res->entry_point = plan->entry_point;
    return 0;
}

static int32_t default_start_process(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    return 0;
}

static int32_t default_request_terminate(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    return 0;
}

static int32_t default_reap_process(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; (void)proc;
    return 0;
}

static bh_pm_kernel_ops_t g_kernel_ops = {
    .ctx = NULL,
    .create_process = default_create_process,
    .create_vm_space = default_create_vm_space,
    .realize_image = default_realize_image,
    .start_process = default_start_process,
    .request_terminate = default_request_terminate,
    .reap_process = default_reap_process
};

void bh_pm_set_kernel_ops(const bh_pm_kernel_ops_t *ops) {
    if (ops) {
        g_kernel_ops = *ops;
    }
}

void bh_pm_set_failure_injection(int fail_stage) {
    g_fail_stage = fail_stage;
}

int bh_pm_register_executable(uint64_t handle, const uint8_t *bytes, size_t size) {
    for (int i = 0; i < MAX_EXECUTABLES; i++) {
        if (!g_executables[i].in_use) {
            g_executables[i].in_use = true;
            g_executables[i].handle = handle;
            g_executables[i].bytes = bytes;
            g_executables[i].size = size;
            return 0;
        }
    }
    return -1;
}

int bh_pm_get_active_count(void) {
    return (int)g_pm_handle_table.count;
}

// Authorization framework
#include <bharat/cap/cap_authz.h>

static const bharat_service_authz_desc_t process_manager_authz_descs[] = {
    {
        .opcode = PM_OP_CREATE,
        .object_type = BHARAT_CAP_OBJ_PROCESS,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = PM_OP_QUERY,
        .object_type = BHARAT_CAP_OBJ_PROCESS,
        .required_rights = BHARAT_CAP_RIGHT_READ,
    },
    {
        .opcode = PM_OP_START,
        .object_type = BHARAT_CAP_OBJ_PROCESS,
        .required_rights = BHARAT_CAP_RIGHT_EXECUTE,
    },
    {
        .opcode = PM_OP_STOP,
        .object_type = BHARAT_CAP_OBJ_PROCESS,
        .required_rights = BHARAT_CAP_RIGHT_EXECUTE,
    }
};

int32_t process_manager_authorize(
    uint32_t opcode,
    const void *req,
    bharat_cap_handle_t caller_cap)
{
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    uint64_t target_object_id = 0;

    switch (opcode) {
        case PM_OP_CREATE: {
            target_object_id = 0;
            break;
        }
        case PM_OP_QUERY: {
            const pm_req_query_t *typed_req = (const pm_req_query_t *)req;
            target_object_id = typed_req->process_id;
            break;
        }
        case PM_OP_START:
        case PM_OP_STOP: {
            const pm_req_start_t *typed_req = (const pm_req_start_t *)req;
            target_object_id = typed_req->process_id;
            break;
        }
        default:
            return BHARAT_IPC_STATUS_ERR_OPCODE;
    }

    return bharat_service_dispatch_authorize(
        PROCESS_MANAGER_SERVICE_ID,
        opcode,
        process_manager_authz_descs,
        sizeof(process_manager_authz_descs) / sizeof(process_manager_authz_descs[0]),
        caller_cap,
        target_object_id
    );
}

void process_manager_init(void) {
    local_memset(process_table, 0, sizeof(process_table));
    local_memset(g_pm_processes, 0, sizeof(g_pm_processes));
    local_memset(g_executables, 0, sizeof(g_executables));

    bh_user_handle_table_init(&g_pm_handle_table, g_pm_handle_slots, MAX_PROCESSES);
    g_fail_stage = 0;
}

int32_t process_manager_handle_create(const pm_req_create_t *req, pm_resp_create_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].in_use) {
            process_table[i].in_use = true;
            process_table[i].process_id = next_pid++;
            process_table[i].executable_id = req->executable_id;
            process_table[i].priority = req->priority;
            process_table[i].state = PM_STATE_CREATED;

            resp->process_id = process_table[i].process_id;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
    return BHARAT_IPC_STATUS_ERR_INTERNAL;
}

int32_t process_manager_handle_start(const pm_req_start_t *req, pm_resp_start_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].in_use && process_table[i].process_id == req->process_id) {
            if (process_table[i].state == PM_STATE_CREATED || process_table[i].state == PM_STATE_STOPPING) {
                process_table[i].state = PM_STATE_RUNNING;
                resp->status = BHARAT_IPC_STATUS_OK;
                return BHARAT_IPC_STATUS_OK;
            } else {
                resp->status = BHARAT_IPC_STATUS_ERR_PERM;
                return BHARAT_IPC_STATUS_ERR_PERM;
            }
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t process_manager_handle_stop(const pm_req_stop_t *req, pm_resp_stop_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].in_use && process_table[i].process_id == req->process_id) {
            if (process_table[i].state == PM_STATE_RUNNING) {
                process_table[i].state = PM_STATE_STOPPING;
                resp->status = BHARAT_IPC_STATUS_OK;
                return BHARAT_IPC_STATUS_OK;
            } else {
                resp->status = BHARAT_IPC_STATUS_ERR_PERM;
                return BHARAT_IPC_STATUS_ERR_PERM;
            }
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t process_manager_handle_query(const pm_req_query_t *req, pm_resp_query_t *resp) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].in_use && process_table[i].process_id == req->process_id) {
            resp->process_id = process_table[i].process_id;
            resp->state = process_table[i].state;
            resp->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

// -----------------------------------------------------------------------------
// v1 APIs Implementation
// -----------------------------------------------------------------------------

int32_t bh_pm_handle_spawn_v1(const bh_pm_spawn_request_v1_t *req, bh_pm_spawn_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_pm_spawn_response_v1_t));
    resp->abi_version = BH_PM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_pm_spawn_response_v1_t);

    if (req->abi_version != BH_PM_INTERFACE_VERSION_V1 || req->struct_size != sizeof(bh_pm_spawn_request_v1_t)) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    // Phase 0: Locate ELF executable image
    const uint8_t *img_bytes = NULL;
    size_t img_size = 0;
    for (int i = 0; i < MAX_EXECUTABLES; i++) {
        if (g_executables[i].in_use && g_executables[i].handle == req->executable_handle) {
            img_bytes = g_executables[i].bytes;
            img_size = g_executables[i].size;
            break;
        }
    }

    if (!img_bytes) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    // Phase 1: Reserve slot in handle table
    bh_pm_process_v1_t *proc = NULL;
    int slot_index = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_pm_processes[i].state == BH_PM_STATE_FREE_V1) {
            proc = &g_pm_processes[i];
            slot_index = i;
            break;
        }
    }

    if (!proc) {
        resp->status = BHARAT_IPC_STATUS_ERR_LENGTH;
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    bh_handle_t proc_handle = 0;
    int h_status = bh_user_handle_alloc(&g_pm_handle_table, proc, BHARAT_CAP_OBJ_PROCESS, BHARAT_CAP_RIGHT_WRITE | BHARAT_CAP_RIGHT_READ | BHARAT_CAP_RIGHT_EXECUTE, &proc_handle);
    if (h_status != BH_HANDLE_TABLE_SUCCESS) {
        resp->status = BHARAT_IPC_STATUS_ERR_LENGTH;
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    proc->process_handle = proc_handle;
    proc->state = BH_PM_STATE_RESERVED_V1;
    local_strncpy(proc->name, req->process_name, 32);
    proc->executable_handle = req->executable_handle;
    proc->priority = req->priority;
    proc->affinity_mask = req->affinity_mask;
    proc->memory_profile = req->memory_profile;
    proc->incarnation_id = 1;

    if (g_fail_stage == 1) {
        // Rollback Phase 1
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Phase 2: Generate ELF load plan
    bh_user_image_plan_v1_t load_plan;
    // Canonical default user ranges: base 0x1000, limit 0x00007FFFFFFFF000
    uint64_t user_base = 0x1000;
    uint64_t user_limit = 0x00007FFFFFFFF000;
    int plan_res = bh_elf_generate_load_plan(img_bytes, img_size, user_base, user_limit, &load_plan);
    if (plan_res != BH_ELF_PLAN_SUCCESS) {
        // Rollback Phase 1
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    proc->state = BH_PM_STATE_PROCESS_CREATED_V1;

    if (g_fail_stage == 2) {
        // Rollback
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Phase 3: Create Process in Kernel
    bh_pm_kernel_create_req_t k_req;
    local_strncpy(k_req.name, proc->name, 32);
    k_req.priority = proc->priority;

    bh_pm_kernel_process_t k_proc;
    int k_res = g_kernel_ops.create_process(g_kernel_ops.ctx, &k_req, &k_proc);
    if (k_res != 0) {
        // Rollback
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    proc->kernel_process_id = k_proc.pid;
    proc->state = BH_PM_STATE_SPACE_CREATED_V1;

    if (g_fail_stage == 3) {
        // Rollback (including call to kernel to stop/destroy)
        g_kernel_ops.reap_process(g_kernel_ops.ctx, &k_proc);
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Phase 4: Create VM Space
    bh_vm_kernel_space_t k_space;
    k_res = g_kernel_ops.create_vm_space(g_kernel_ops.ctx, &k_proc, proc->memory_profile, &k_space);
    if (k_res != 0) {
        // Rollback
        g_kernel_ops.reap_process(g_kernel_ops.ctx, &k_proc);
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    proc->vm_space_handle = k_space.space_id;
    proc->state = BH_PM_STATE_IMAGE_REALIZED_V1;

    if (g_fail_stage == 4) {
        // Rollback
        g_kernel_ops.reap_process(g_kernel_ops.ctx, &k_proc);
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Phase 5: Realize image PT_LOAD segments & stack
    bh_pm_kernel_image_result_t k_img_res;
    k_res = g_kernel_ops.realize_image(g_kernel_ops.ctx, &k_proc, &k_space, &load_plan, &k_img_res);
    if (k_res != 0) {
        // Rollback
        g_kernel_ops.reap_process(g_kernel_ops.ctx, &k_proc);
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    proc->main_thread_id = k_img_res.main_thread_id;
    proc->state = BH_PM_STATE_THREAD_CREATED_V1;

    if (g_fail_stage == 5) {
        // Rollback
        g_kernel_ops.reap_process(g_kernel_ops.ctx, &k_proc);
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Phase 6: Resume/start initial user thread
    k_res = g_kernel_ops.start_process(g_kernel_ops.ctx, &k_proc);
    if (k_res != 0) {
        // Rollback
        g_kernel_ops.reap_process(g_kernel_ops.ctx, &k_proc);
        bh_user_handle_revoke(&g_pm_handle_table, proc_handle);
        local_memset(proc, 0, sizeof(bh_pm_process_v1_t));
        resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    proc->state = BH_PM_STATE_RUNNING_V1;

    // Set response values
    resp->status = BHARAT_IPC_STATUS_OK;
    resp->initial_state = proc->state;
    resp->process_handle = proc_handle;
    resp->vm_space_handle = proc->vm_space_handle;
    resp->kernel_process_id = proc->kernel_process_id;
    resp->main_thread_id = proc->main_thread_id;
    resp->incarnation_id = proc->incarnation_id;

    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_pm_handle_query_v1(const bh_pm_query_request_v1_t *req, bh_pm_query_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_pm_query_response_v1_t));
    resp->abi_version = BH_PM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_pm_query_response_v1_t);

    bh_pm_process_v1_t *proc = NULL;
    int h_res = bh_user_handle_lookup(&g_pm_handle_table, req->process_handle, BHARAT_CAP_OBJ_PROCESS, (void **)&proc, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !proc) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    resp->status = BHARAT_IPC_STATUS_OK;
    resp->state = proc->state;
    resp->kernel_process_id = proc->kernel_process_id;
    resp->main_thread_id = proc->main_thread_id;
    resp->exit_code = proc->exit_code;
    resp->exit_reason = proc->exit_reason;

    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_pm_handle_terminate_v1(const bh_pm_terminate_request_v1_t *req, bh_pm_terminate_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_pm_terminate_response_v1_t));
    resp->abi_version = BH_PM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_pm_terminate_response_v1_t);

    bh_pm_process_v1_t *proc = NULL;
    int h_res = bh_user_handle_lookup(&g_pm_handle_table, req->process_handle, BHARAT_CAP_OBJ_PROCESS, (void **)&proc, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !proc) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    // Idempotent terminate
    if (proc->state == BH_PM_STATE_EXITED_V1 || proc->state == BH_PM_STATE_REAPED_V1) {
        resp->status = BHARAT_IPC_STATUS_OK;
        return BHARAT_IPC_STATUS_OK;
    }

    proc->state = BH_PM_STATE_TERMINATE_REQUESTED_V1;
    bh_pm_kernel_process_t k_proc = { .pid = proc->kernel_process_id };
    g_kernel_ops.request_terminate(g_kernel_ops.ctx, &k_proc);

    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

int32_t bh_pm_handle_wait_v1(const bh_pm_wait_request_v1_t *req, bh_pm_wait_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_pm_wait_response_v1_t));
    resp->abi_version = BH_PM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_pm_wait_response_v1_t);

    bh_pm_process_v1_t *proc = NULL;
    int h_res = bh_user_handle_lookup(&g_pm_handle_table, req->process_handle, BHARAT_CAP_OBJ_PROCESS, (void **)&proc, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !proc) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (proc->state == BH_PM_STATE_EXITED_V1) {
        resp->status = BHARAT_IPC_STATUS_OK;
        resp->exit_code = proc->exit_code;
        resp->exit_reason = proc->exit_reason;
        return BHARAT_IPC_STATUS_OK;
    }

    if (req->wait_flags & BH_PM_WAIT_NONBLOCK) {
        resp->status = BHARAT_IPC_STATUS_ERR_BUSY; // Or custom wait-not-ready
        return BHARAT_IPC_STATUS_ERR_BUSY;
    }

    // Register a waiter
    proc->has_waiter = true;
    proc->waiter_flags = req->wait_flags;
    proc->waiter_timeout_ms = req->timeout_ms;

    resp->status = BHARAT_IPC_STATUS_ERR_BUSY; // Busy because it's still running/waiting
    return BHARAT_IPC_STATUS_ERR_BUSY;
}

int32_t bh_pm_handle_reap_v1(const bh_pm_reap_request_v1_t *req, bh_pm_reap_response_v1_t *resp) {
    if (!req || !resp) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    local_memset(resp, 0, sizeof(bh_pm_reap_response_v1_t));
    resp->abi_version = BH_PM_INTERFACE_VERSION_V1;
    resp->struct_size = sizeof(bh_pm_reap_response_v1_t);

    bh_pm_process_v1_t *proc = NULL;
    int h_res = bh_user_handle_lookup(&g_pm_handle_table, req->process_handle, BHARAT_CAP_OBJ_PROCESS, (void **)&proc, NULL);
    if (h_res != BH_HANDLE_TABLE_SUCCESS || !proc) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    // Must be in zombie EXITED or FAILED state to reap
    if (proc->state != BH_PM_STATE_EXITED_V1 && proc->state != BH_PM_STATE_FAILED_V1 && proc->state != BH_PM_STATE_THREAD_CREATED_V1) {
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    bh_pm_kernel_process_t k_proc = { .pid = proc->kernel_process_id };
    g_kernel_ops.reap_process(g_kernel_ops.ctx, &k_proc);

    proc->state = BH_PM_STATE_REAPED_V1;

    // Revoke the handle in user-space handle table
    bh_user_handle_revoke(&g_pm_handle_table, req->process_handle);

    // Completely zero and clean the slot so baseline process counts decrease
    local_memset(proc, 0, sizeof(bh_pm_process_v1_t));

    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

void bh_pm_notify_exit_v1(uint64_t kernel_process_id, int32_t exit_code, uint32_t exit_reason) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_pm_processes[i].state != BH_PM_STATE_FREE_V1 && g_pm_processes[i].kernel_process_id == kernel_process_id) {
            g_pm_processes[i].state = BH_PM_STATE_EXITED_V1;
            g_pm_processes[i].exit_code = exit_code;
            g_pm_processes[i].exit_reason = exit_reason;
            g_pm_processes[i].has_waiter = false;
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// IPC Dispatch Loop for versioned UAPI
// -----------------------------------------------------------------------------

void process_manager_loop(bharat_ipc_endpoint_t endpoint) {
    bharat_ipc_msg_header_t req_header;
    bharat_ipc_msg_header_t resp_header;
    uint8_t payload_buf[512];
    uint8_t resp_payload_buf[512];

    while (true) {
        int32_t recv_status = bharat_ipc_recv(endpoint, &req_header, payload_buf, sizeof(payload_buf));
        if (recv_status < 0) {
            continue;
        }

        if (req_header.service_id != PROCESS_MANAGER_SERVICE_ID) {
            continue;
        }

        resp_header.service_id = req_header.service_id;
        resp_header.interface_id = req_header.interface_id;
        resp_header.interface_version = req_header.interface_version;
        resp_header.opcode = req_header.opcode;
        resp_header.message_id = req_header.message_id;

        int32_t dispatch_status = BHARAT_IPC_STATUS_ERR_OPCODE;
        uint32_t resp_size = 0;

        // Route between legacy v0 and modern v1 interface versions
        if (req_header.interface_version == BH_PM_INTERFACE_VERSION_V1) {
            switch (req_header.opcode) {
                case BH_PM_OP_SPAWN_V1: {
                    if (req_header.payload_size >= sizeof(bh_pm_spawn_request_v1_t)) {
                        const bh_pm_spawn_request_v1_t *req = (const bh_pm_spawn_request_v1_t *)payload_buf;
                        bh_pm_spawn_response_v1_t *resp = (bh_pm_spawn_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_pm_handle_spawn_v1(req, resp);
                        resp_size = sizeof(bh_pm_spawn_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_PM_OP_QUERY_V1: {
                    if (req_header.payload_size >= sizeof(bh_pm_query_request_v1_t)) {
                        const bh_pm_query_request_v1_t *req = (const bh_pm_query_request_v1_t *)payload_buf;
                        bh_pm_query_response_v1_t *resp = (bh_pm_query_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_pm_handle_query_v1(req, resp);
                        resp_size = sizeof(bh_pm_query_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_PM_OP_REQUEST_TERMINATE_V1: {
                    if (req_header.payload_size >= sizeof(bh_pm_terminate_request_v1_t)) {
                        const bh_pm_terminate_request_v1_t *req = (const bh_pm_terminate_request_v1_t *)payload_buf;
                        bh_pm_terminate_response_v1_t *resp = (bh_pm_terminate_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_pm_handle_terminate_v1(req, resp);
                        resp_size = sizeof(bh_pm_terminate_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_PM_OP_WAIT_V1: {
                    if (req_header.payload_size >= sizeof(bh_pm_wait_request_v1_t)) {
                        const bh_pm_wait_request_v1_t *req = (const bh_pm_wait_request_v1_t *)payload_buf;
                        bh_pm_wait_response_v1_t *resp = (bh_pm_wait_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_pm_handle_wait_v1(req, resp);
                        resp_size = sizeof(bh_pm_wait_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case BH_PM_OP_REAP_V1: {
                    if (req_header.payload_size >= sizeof(bh_pm_reap_request_v1_t)) {
                        const bh_pm_reap_request_v1_t *req = (const bh_pm_reap_request_v1_t *)payload_buf;
                        bh_pm_reap_response_v1_t *resp = (bh_pm_reap_response_v1_t *)resp_payload_buf;
                        dispatch_status = bh_pm_handle_reap_v1(req, resp);
                        resp_size = sizeof(bh_pm_reap_response_v1_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                default:
                    break;
            }
        } else {
            // Legacy v0 interface
            switch (req_header.opcode) {
                case PM_OP_CREATE: {
                    if (req_header.payload_size >= sizeof(pm_req_create_t)) {
                        pm_req_create_t *req = (pm_req_create_t*)payload_buf;
                        pm_resp_create_t *resp = (pm_resp_create_t*)resp_payload_buf;
                        int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                        if (auth_status != BHARAT_IPC_STATUS_OK) {
                            resp->status = auth_status;
                            dispatch_status = auth_status;
                            resp_size = sizeof(pm_resp_create_t);
                            break;
                        }
                        dispatch_status = process_manager_handle_create(req, resp);
                        resp_size = sizeof(pm_resp_create_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case PM_OP_START: {
                    if (req_header.payload_size >= sizeof(pm_req_start_t)) {
                        pm_req_start_t *req = (pm_req_start_t*)payload_buf;
                        pm_resp_start_t *resp = (pm_resp_start_t*)resp_payload_buf;
                        int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                        if (auth_status != BHARAT_IPC_STATUS_OK) {
                            resp->status = auth_status;
                            dispatch_status = auth_status;
                            resp_size = sizeof(pm_resp_start_t);
                            break;
                        }
                        dispatch_status = process_manager_handle_start(req, resp);
                        resp_size = sizeof(pm_resp_start_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case PM_OP_STOP: {
                    if (req_header.payload_size >= sizeof(pm_req_stop_t)) {
                        pm_req_stop_t *req = (pm_req_stop_t*)payload_buf;
                        pm_resp_stop_t *resp = (pm_resp_stop_t*)resp_payload_buf;
                        int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                        if (auth_status != BHARAT_IPC_STATUS_OK) {
                            resp->status = auth_status;
                            dispatch_status = auth_status;
                            resp_size = sizeof(pm_resp_stop_t);
                            break;
                        }
                        dispatch_status = process_manager_handle_stop(req, resp);
                        resp_size = sizeof(pm_resp_stop_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                case PM_OP_QUERY: {
                    if (req_header.payload_size >= sizeof(pm_req_query_t)) {
                        pm_req_query_t *req = (pm_req_query_t*)payload_buf;
                        pm_resp_query_t *resp = (pm_resp_query_t*)resp_payload_buf;
                        int32_t auth_status = process_manager_authorize(req_header.opcode, req, req_header.capability_transfer);
                        if (auth_status != BHARAT_IPC_STATUS_OK) {
                            resp->status = auth_status;
                            dispatch_status = auth_status;
                            resp_size = sizeof(pm_resp_query_t);
                            break;
                        }
                        dispatch_status = process_manager_handle_query(req, resp);
                        resp_size = sizeof(pm_resp_query_t);
                    } else {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        resp_header.payload_size = resp_size;
        resp_header.flags = dispatch_status;

        if (req_header.reply_endpoint != 0) {
            bharat_ipc_endpoint_t rep_ep = req_header.reply_endpoint;
            bharat_ipc_send(rep_ep, &resp_header, resp_payload_buf);
        }
    }
}
