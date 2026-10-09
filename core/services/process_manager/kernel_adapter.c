#include "process_manager.h"
#include <bharat/runtime/runtime.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <bharat/uapi/syscall_args.h>

static int invoke(uint32_t cap, uint32_t op, const void *req, void *out) {
    bharat_sys_cap_invoke_args_t args = {.cap_id = cap, .opcode = op,
        .arg0 = (uintptr_t)req, .arg1 = (uintptr_t)out};
    int status = bharat_syscall(BH_SYS_CAPABILITY_INVOKE, (uintptr_t)&args, 0, 0, 0, 0, 0);
    return status == 0 ? BHARAT_IPC_STATUS_OK : BHARAT_IPC_STATUS_ERR_INTERNAL;
}

static bharat_status_t native_create(void *ctx, const bh_pm_kernel_create_req_t *req,
                                     bh_pm_kernel_process_t *out) {
    (void)ctx;
    if (!req || !out || !req->image_bytes || !req->image_size) return BHARAT_IPC_STATUS_ERR_INVALID;
    bh_process_image_request_t request = {.version = BH_BOOTSTRAP_SERVICE_ABI,
        .priority = req->priority, .image_address = (uintptr_t)req->image_bytes,
        .image_size = req->image_size};
    for (size_t i = 0; i < sizeof(request.name) - 1; ++i) request.name[i] = req->name[i];
    bh_process_image_result_t result = {0};
    int status = invoke(bharat_runtime_get_bootstrap_cap(), BH_BOOTSTRAP_OP_CREATE_IMAGE, &request, &result);
    if (status == 0) { out->pid = result.process_id; out->cap = result.process_cap; }
    return status;
}

static bharat_status_t native_space(void *ctx, bh_pm_kernel_process_t *proc,
                                    uint32_t profile, bh_vm_kernel_space_t *out) {
    (void)ctx;
    if (profile != 0) return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
    bh_process_image_result_t result = {0};
    int status = invoke(proc->cap, BH_PROCESS_OP_QUERY, NULL, &result);
    if (status == 0 && result.process_id == proc->pid) out->space_id = result.process_id;
    else return BHARAT_IPC_STATUS_ERR_INTERNAL;
    return status;
}

static bharat_status_t native_image(void *ctx, bh_pm_kernel_process_t *proc,
    bh_vm_kernel_space_t *space, const bh_user_image_plan_v1_t *plan, bh_pm_kernel_image_result_t *out) {
    (void)ctx;
    if (space->space_id != proc->pid) return BHARAT_IPC_STATUS_ERR_INVALID;
    bh_process_image_result_t result = {0};
    int status = invoke(proc->cap, BH_PROCESS_OP_QUERY, NULL, &result);
    if (status == 0 && result.entry_point == plan->entry_point && result.process_id == proc->pid) {
        out->entry_point = result.entry_point;
        out->main_thread_id = result.thread_id;
    } else return BHARAT_IPC_STATUS_ERR_INTERNAL;
    return status;
}

static bharat_status_t native_start(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; return invoke(proc->cap, BH_PROCESS_OP_START, NULL, NULL);
}
static bharat_status_t native_terminate(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; return invoke(proc->cap, BH_PROCESS_OP_TERMINATE, NULL, NULL);
}
static bharat_status_t native_reap(void *ctx, bh_pm_kernel_process_t *proc) {
    (void)ctx; return invoke(proc->cap, BH_PROCESS_OP_REAP, NULL, NULL);
}

bharat_status_t bh_pm_install_native_kernel_ops(void) {
    if (bharat_bootstrap_probe() != 0) return BHARAT_IPC_STATUS_ERR_PERM;
    const bh_pm_kernel_ops_t ops = {.create_process = native_create, .create_vm_space = native_space,
        .realize_image = native_image, .start_process = native_start,
        .request_terminate = native_terminate, .reap_process = native_reap};
    return bh_pm_set_kernel_ops(&ops);
}
