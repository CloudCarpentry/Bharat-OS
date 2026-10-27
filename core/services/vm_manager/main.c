#include <stdint.h>
#include "vm_manager.h"
#include <bharat/runtime/runtime.h>
#include <bharat/uapi/init/bootstrap.h>

int main(int argc, char** argv) {
    bharat_runtime_log("VM_MANAGER_LAUNCH\n");
    (void)argc;
    (void)argv;

    vm_manager_init();

    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    bharat_ipc_endpoint_t endpoint = startup ? startup->bootstrap.service_receive_endpoint : 0;

    if (!endpoint) {
        (void)bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_FAILED, BHARAT_IPC_STATUS_ERR_NOT_FOUND);
        return 1;
    }

    if (bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_BOUND, 0) != 0) return 1;

    if (!bh_vm_authority_ops_installed()) {
        bharat_runtime_log("VM_MANAGER_BLOCKED: KERNEL_VM_UAPI_MISSING\n");
        (void)bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_FAILED, BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
        return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
    }

    // Additional profile validation can be done here if necessary, but the
    // requirement specifies not to introduce a competing hardware capability model.
    // The core validation logic should reside in the VM manager's operations.

    if (bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_READY, 0) != 0) return 1;
    bharat_runtime_log("VM_MANAGER_READY\n");

    vm_manager_loop(endpoint);

    return 0;
}