#include <stdint.h>
#include "servicemgr.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    servicemgr_init();

    bharat_ipc_endpoint_t endpoint = 0x3000; // Fake endpoint for now

    servicemgr_loop(endpoint);

    return 0;
}
