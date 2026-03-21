#include <bharat/runtime/runtime.h>
#include <bharat/ipc/ipc.h>
#include "src/registry.h"
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

// Custom memset and memcpy
static void *custom_memset_main(void *s, int c, unsigned long n) {
    unsigned char *p = s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}

static void *custom_memcpy_main(void *dest, const void *src, unsigned long n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}


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

    bharat_ipc_msg_header_t hdr;
    namesvc_ipc_req_t req;
    namesvc_ipc_res_t res;

    // Simulate main IPC loop handling registration/lookup requests
    while(1) {
        custom_memset_main(&hdr, 0, sizeof(hdr));
        custom_memset_main(&req, 0, sizeof(req));
        custom_memset_main(&res, 0, sizeof(res));

        int ret = bharat_ipc_recv(my_endpoint, &hdr, &req, sizeof(req));

        // As IPC is not fully implemented in stubs, this would block.
        // We'll process requests if they arrive.
        if (ret == 0) {
            res.status = NAMESVC_STATUS_ERR_INVAL;

            switch (req.opcode) {
                case NAMESVC_OP_REGISTER:
                    if (bharat_cap_is_valid(hdr.capability_transfer)) {
                        res.status = namesvc_registry_add(req.u.reg.name, hdr.capability_transfer);
                        if (res.status == NAMESVC_STATUS_OK) {
                            bharat_runtime_log("services/namesvc: Registered endpoint.");
                        }
                    } else {
                        res.status = NAMESVC_STATUS_ERR_INVAL;
                    }
                    break;

                case NAMESVC_OP_LOOKUP:
                    res.status = namesvc_registry_lookup(req.u.lookup.name, &res.u.lookup_res.endpoint);
                    if (res.status == NAMESVC_STATUS_OK) {
                        bharat_runtime_log("services/namesvc: Lookup successful.");
                    }
                    break;

                case NAMESVC_OP_REMOVE:
                    res.status = namesvc_registry_remove(req.u.remove.name);
                    break;

                default:
                    res.status = NAMESVC_STATUS_ERR_INVAL;
                    break;
            }

            bharat_ipc_msg_header_t rep_hdr;
            custom_memset_main(&rep_hdr, 0, sizeof(rep_hdr));
            rep_hdr.message_id = hdr.message_id;
            rep_hdr.payload_size = sizeof(res);

            if (req.opcode == NAMESVC_OP_LOOKUP && res.status == NAMESVC_STATUS_OK) {
                rep_hdr.capability_transfer = res.u.lookup_res.endpoint;
            }

            bharat_ipc_send(hdr.capability_transfer /* assuming reply cap is passed here in a real system */, &rep_hdr, &res);
        } else {
             // For stub compilation, we just break to avoid infinite loop taking 100% CPU when recv doesn't block
             break;
        }
    }

    bharat_runtime_shutdown();
    return 0;
}
