#include <stdint.h>
#include "process_manager.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    process_manager_init();

    /* Refuse to advertise a process service without real kernel authority. */
    if (!bh_pm_kernel_ops_installed()) {
        return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
    }

    bharat_ipc_endpoint_t endpoint = 0x1000; // Fake endpoint for now

    process_manager_loop(endpoint);

    return 0;
}
