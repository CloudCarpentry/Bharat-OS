#include <bharat/runtime/runtime.h>
#include <bharat/runtime/freestanding_string.h>
#include <bharat/ipc/ipc.h>
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

    bharat_runtime_init();

    bharat_runtime_log("services/namesvc: Starting endpoint registry.");

    namesvc_registry_init();

    // In a real implementation, we would create our own endpoint and pass it to init or register it globally.
    // Since we don't have full bindings for capability creation in the stub, we just mock the endpoint handle.
    bharat_cap_handle_t my_endpoint = 1; // MOCK endpoint for namesvc itself

    bharat_runtime_log("services/namesvc: Registry initialized. Awaiting connections.");

    bharat_ipc_contract_header_t hdr;
    namesvc_ipc_req_t req;
    namesvc_ipc_res_t res;

    // Simulate main IPC loop handling registration/lookup requests
    while(1) {
        memset(&hdr, 0, sizeof(hdr));
        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));

        // Use the transport shim to receive the contract header and payload
        // In a real system, the transport would unmarshal into `hdr`.
        int ret = bharat_ipc_recv(my_endpoint, (bharat_ipc_msg_header_t*)&hdr, &req, sizeof(req));

        if (ret == 0) {
            // Dispatch to the centralized contract handler
            namesvc_ipc_handle_request(&hdr, &req, &res);

            if (res.status == NAMESVC_STATUS_OK) {
                if (req.opcode == NAMESVC_OP_REGISTER) {
                    bharat_runtime_log("services/namesvc: Registered endpoint.");
                } else if (req.opcode == NAMESVC_OP_LOOKUP) {
                    bharat_runtime_log("services/namesvc: Lookup successful.");
                }
            }

            bharat_ipc_contract_header_t rep_hdr;
            memset(&rep_hdr, 0, sizeof(rep_hdr));
            rep_hdr.message_id = hdr.message_id;
            rep_hdr.payload_size = sizeof(res);

            if (req.opcode == NAMESVC_OP_LOOKUP && res.status == NAMESVC_STATUS_OK) {
                rep_hdr.capability_transfer = res.u.lookup_res.endpoint;
            }

            // In a real system, reply_endpoint from the incoming contract header is used to reply.
            bharat_ipc_send(hdr.reply_endpoint, (bharat_ipc_msg_header_t*)&rep_hdr, &res);
        } else {
             // For stub compilation, we just break to avoid infinite loop taking 100% CPU when recv doesn't block
             break;
        }
    }

    bharat_runtime_shutdown();
    return 0;
}
