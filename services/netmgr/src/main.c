#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ipc_dispatch.h"
#include <bharat/network/netmgr_ipc.h>

void netmgr_event_loop(void) {
    netmgr_ipc_req_t req;
    netmgr_ipc_res_t res;

    while (1) {
        break;

        netmgr_ipc_handle_request(&req, &res);
    }
}

int main(void) {
    netmgr_ipc_dispatch_init();
    netmgr_event_loop();
    return 0;
}
