#include <stdint.h>
#include "process_manager.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    process_manager_init();

    bharat_ipc_endpoint_t endpoint = 0x1000; // Fake endpoint for now

    process_manager_loop(endpoint);

    return 0;
}
