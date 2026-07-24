#include <bharat/service/service_runtime.h>
#include <bharat/runtime/freestanding_string.h>
#include <bharat/ipc/ipc.h>
#include <bharat/uapi/services/bootstrap.h>
#include "src/registry.h"
#include "include/ipc_dispatch.h"
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

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // namesvc uses a minimal bootstrap instead of full bharat_runtime_init()
    // to avoid circular dependencies with services it provides.

    // Create our endpoint
    bharat_ipc_endpoint_t my_endpoint = service_runtime_create_endpoint(BHARAT_SERVICE_NAMESVC, 0);
    if (!bharat_cap_is_valid(my_endpoint)) {
        return -1;
    }

    // Bind to the well-known bootstrap handle
    if (service_runtime_bind_namesvc_bootstrap(my_endpoint) != BHARAT_STATUS_OK) {
        return -1;
    }

    namesvc_registry_init();

    // Use a simple log since we might not have a full logger yet
    // bharat_runtime_log("namesvc: ready");

    bharat_ipc_msg_header_t hdr;
    namesvc_ipc_req_t req;
    namesvc_ipc_res_t res;

    while(1) {
        memset(&hdr, 0, sizeof(hdr));
        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        int ret = bharat_ipc_recv(my_endpoint, &hdr, &req, sizeof(req));

        if (ret == BHARAT_IPC_STATUS_OK) {
            namesvc_ipc_handle_request(&hdr, &req, &res);

            bharat_ipc_msg_header_t rep_hdr;
            memset(&rep_hdr, 0, sizeof(rep_hdr));
            rep_hdr.header_version = BHARAT_IPC_HEADER_VERSION_V1;
            rep_hdr.message_id = hdr.message_id;
            rep_hdr.service_id = BHARAT_SERVICE_NAMESVC;
            rep_hdr.opcode = req.opcode;
            rep_hdr.payload_size = sizeof(res);

            if (req.opcode == BHARAT_NAMESVC_OP_LOOKUP && res.status == NAMESVC_STATUS_OK) {
                rep_hdr.capability_transfer = res.u.lookup_res.endpoint;
            }

            if (bharat_cap_is_valid(hdr.reply_endpoint)) {
                bharat_ipc_send(hdr.reply_endpoint, &rep_hdr, &res);
            }
        } else {
             // For Phase A, we yield if no message
             bharat_sched_yield();
        }
    }

    return 0;
}
