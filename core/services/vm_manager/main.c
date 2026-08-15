#include <stdint.h>
#include "vm_manager.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    vm_manager_init();

    /* Do not expose a VM service backed only by the fail-closed adapter. */
    if (!bh_vm_authority_ops_installed()) {
        return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
    }

    bharat_ipc_endpoint_t endpoint = 0x2000; // Fake endpoint for now

    vm_manager_loop(endpoint);

    return 0;
}
