#include <stdint.h>
#include "vm_manager.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    vm_manager_init();

    bharat_ipc_endpoint_t endpoint = 0x2000; // Fake endpoint for now

    vm_manager_loop(endpoint);

    return 0;
}
