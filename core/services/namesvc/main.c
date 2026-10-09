#include <bharat/uapi/service_status.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/runtime/runtime.h>
#include <bharat/syscalls.h>
#include <bharat/runtime/freestanding_string.h>
#include <bharat/ipc/ipc.h>
#include <bharat/uapi/services/bootstrap.h>
#include "src/registry.h"
#include <bharat/uapi/init/bootstrap.h>
extern const bharat_user_startup_t *bharat_runtime_get_startup(void);
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

static bharat_status_t namesvc_run(void) {
    // namesvc uses a minimal bootstrap instead of full bharat_runtime_init()
    // to avoid circular dependencies with services it provides.

    // Create our endpoint
    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    bharat_ipc_endpoint_t my_endpoint = BHARAT_CAP_INVALID_HANDLE;
    if (startup && startup->bootstrap.service_receive_endpoint) {
        my_endpoint = startup->bootstrap.service_receive_endpoint;
    }
    if (!bharat_cap_is_valid(my_endpoint)) {
        return BHARAT_STATUS_ERR_NOT_FOUND;
    }

    // Bind to the well-known bootstrap handle
    /* The launcher supplies a child-local receive capability for this server.
     * Never manufacture a well-known integer handle or alias a parent CSpace. */
    bharat_runtime_log("BOOTAUTH:NAMESVC_BINDING_OK\n");

    namesvc_registry_init();
    if (bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_BOUND, 0) != 0 ||
        bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_READY, 0) != 0) return BHARAT_STATUS_ERR_NOT_FOUND;
    bharat_runtime_log("NAMESVC_READY\n");

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

    return BHARAT_STATUS_OK;
}

int main(int argc, char **argv) {
    bharat_runtime_log("NAMESVC_MAIN_ENTER\n");
    (void)argc;
    (void)argv;

    bharat_status_t run_status = namesvc_run();
    if (run_status != BHARAT_STATUS_OK) {
        (void)bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_FAILED, run_status);
        // Map service status failure to non-zero exit code
        return 1;
    }

    return 0;
}
