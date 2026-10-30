#include <bharat/uapi/service_status.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/runtime/runtime.h>
#include <bharat/syscalls.h>
#include <bharat/runtime/freestanding_string.h>
#include <bharat/ipc/ipc.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/services/bootstrap.h>
#include "src/registry.h"
#include <bharat/uapi/init/bootstrap.h>
#include "include/ipc_dispatch.h"
#include "bharat/component_version.h"
#include "bharat/buildinfo.h"

extern const bharat_user_startup_t *bharat_runtime_get_startup(void);

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

static bh_ipc_receive_envelope_v1_t g_current_env;

static bharat_cap_status_t namesvc_envelope_resolver(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)expected_object_type;
    (void)expected_object_id;
    (void)required_rights;
    (void)required_scope;
    if (out_result) {
        memset(out_result, 0, sizeof(*out_result));
    }
    if (!bharat_cap_is_valid(handle)) {
        if (out_result) out_result->status = BHARAT_CAP_INVALID;
        return BHARAT_CAP_INVALID;
    }
    if (out_result) {
        out_result->allowed = true;
        out_result->status = BHARAT_CAP_OK;
        out_result->descriptor.owner_principal_id =
            (g_current_env.authority_cap == handle && g_current_env.sender_pid != 0U)
                ? g_current_env.sender_pid
                : (uint64_t)handle;
    }
    return BHARAT_CAP_OK;
}

static bharat_status_t namesvc_run(void) {
    // namesvc uses a minimal bootstrap instead of full bharat_runtime_init()
    // to avoid circular dependencies with services it provides.

    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    bharat_ipc_endpoint_t my_endpoint = BHARAT_CAP_INVALID_HANDLE;
    if (startup && startup->bootstrap.service_receive_endpoint) {
        my_endpoint = startup->bootstrap.service_receive_endpoint;
    }
    if (!bharat_cap_is_valid(my_endpoint)) {
        return BHARAT_STATUS_ERR_NOT_FOUND;
    }

    /* The launcher supplies a child-local receive capability for this server.
     * Never manufacture a well-known integer handle or alias a parent CSpace. */
    bharat_runtime_log("BOOTAUTH:NAMESVC_BINDING_OK\n");

    bharat_cap_register_authority_resolver(namesvc_envelope_resolver);
    namesvc_registry_init();
    if (bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_BOUND, 0) != 0 ||
        bharat_bootstrap_report(BH_BOOTSTRAP_EVENT_READY, 0) != 0) return BHARAT_STATUS_ERR_NOT_FOUND;
    bharat_runtime_log("NAMESVC_READY\n");

    bharat_ipc_msg_header_t hdr;
    namesvc_ipc_req_t req;
    namesvc_ipc_res_t res;

    while (1) {
        memset(&hdr, 0, sizeof(hdr));
        memset(&req, 0, sizeof(req));
        memset(&res, 0, sizeof(res));
        memset(&g_current_env, 0, sizeof(g_current_env));

        int ret = bharat_ipc_recv_envelope(my_endpoint, &hdr, &req, sizeof(req), &g_current_env);

        if (ret == BHARAT_IPC_STATUS_OK) {
            /* Populate capability_transfer exclusively from kernel-verified authority_cap.
             * Never use env.reply_send_cap as authority. */
            hdr.capability_transfer = g_current_env.authority_cap;
            namesvc_ipc_handle_request(&hdr, &req, &res);

            if ((req.opcode == BHARAT_NAMESVC_OP_REGISTER || req.opcode == BHARAT_NAMESVC_OP_UNREGISTER) &&
                res.status != BHARAT_IPC_STATUS_OK &&
                bharat_cap_is_valid(g_current_env.authority_cap)) {
                (void)bharat_ipc_endpoint_close(g_current_env.authority_cap);
            }

            bharat_ipc_msg_header_t rep_hdr;
            memset(&rep_hdr, 0, sizeof(rep_hdr));
            rep_hdr.header_version = BHARAT_IPC_HEADER_VERSION_V1;
            rep_hdr.message_id = hdr.message_id;
            rep_hdr.service_id = BHARAT_SERVICE_NAMESVC;
            rep_hdr.opcode = req.opcode;
            rep_hdr.payload_size = sizeof(res);

            if (bharat_cap_is_valid(g_current_env.reply_send_cap)) {
                if (req.opcode == BHARAT_NAMESVC_OP_LOOKUP &&
                    res.status == NAMESVC_STATUS_OK &&
                    bharat_cap_is_valid(res.u.lookup_res.endpoint)) {
                    (void)bharat_ipc_send_cap(g_current_env.reply_send_cap,
                                              &rep_hdr,
                                              &res,
                                              res.u.lookup_res.endpoint,
                                              0U);
                } else {
                    (void)bharat_ipc_send(g_current_env.reply_send_cap, &rep_hdr, &res);
                }
                (void)bharat_ipc_endpoint_close(g_current_env.reply_send_cap);
            }
        } else {
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
        return 1;
    }

    return 0;
}
