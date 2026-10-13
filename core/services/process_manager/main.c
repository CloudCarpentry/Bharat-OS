#include <stdint.h>
#include "process_manager.h"
#include <bharat/runtime/runtime.h>
#include <bharat/uapi/init/bootstrap.h>

int main(int argc, char** argv) {
    bharat_runtime_log("PROCESS_MANAGER_LAUNCH\n");
    (void)argc;
    (void)argv;

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
