#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ipc_dispatch.h"
#include <bharat/network/netmgr_ipc.h>
#include "bharat/component_version.h"
#include "bharat/buildinfo.h"

BHARAT_REGISTER_COMPONENT(
    BHARAT_COMPONENT_NAME,
    BHARAT_COMPONENT_KIND,
    BHARAT_COMPONENT_VERSION,
    BHARAT_COMPONENT_IFACE,
    0, /* abi version */
    BHARAT_COMPONENT_CHANNEL,
    BHARAT_GIT_SHA,
    BHARAT_GIT_DIRTY,
    BHARAT_BUILD_EPOCH,
    BHARAT_BUILD_TIME_UTC
);

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
