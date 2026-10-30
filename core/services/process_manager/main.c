#include <stdint.h>
#include "process_manager.h"
#include <bharat/runtime/runtime.h>
#include <bharat/uapi/init/bootstrap.h>
#include <bharat/cap/cap_validate.h>

static bharat_cap_status_t pm_envelope_resolver(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)expected_object_id;
    (void)required_rights;
    (void)required_scope;
    if (!out_result) return BHARAT_CAP_INVALID;
    out_result->allowed = false;
    out_result->status = BHARAT_CAP_INVALID;
    if (handle == BHARAT_CAP_INVALID_HANDLE || expected_object_type != BHARAT_CAP_OBJ_PROCESS) {
        return BHARAT_CAP_INVALID;
    }
    out_result->allowed = true;
    out_result->status = BHARAT_CAP_OK;
    return BHARAT_CAP_OK;
}

int main(int argc, char** argv) {
    bharat_runtime_log("PROCESS_MANAGER_LAUNCH\n");
    (void)argc;
    (void)argv;

    (void)bharat_cap_register_authority_resolver(pm_envelope_resolver);

    process_manager_init();
    if (bh_pm_install_native_kernel_ops() != BHARAT_IPC_STATUS_OK) {
        (void)bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_FAILED, BHARAT_IPC_STATUS_ERR_PERM);
        return 1;
    }

    /* Refuse to advertise a process service without real kernel authority. */
    if (!bh_pm_kernel_ops_installed()) {
        return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
    }

    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    bharat_ipc_endpoint_t endpoint = startup ? startup->bootstrap.service_receive_endpoint : 0;
    if (!endpoint || bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_BOUND, 0) != 0 ||
        bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_READY, 0) != 0) return 1;
    bharat_runtime_log("PROCESS_MANAGER_READY\n");

    process_manager_loop(endpoint);

    return 0;
}
