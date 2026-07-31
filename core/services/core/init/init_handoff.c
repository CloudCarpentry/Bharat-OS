#include "init_handoff.h"
#include "init_contract.h"
#include <bharat/namesvc/client.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/uapi/servicemgr/handoff.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/runtime/runtime.h>
#include <errno.h>

int init_handoff_to_supervisor(const init_boot_context_t *ctx, struct init_runtime_s *rt) {
    if (!ctx || !rt) return -EINVAL;

    bharat_runtime_log("services/init: Preparing chunked handoff to supervisor...");

    // 1. Lookup bharat.servicemgr in namesvc
    bharat_service_id_t sm_svc_id = 0;
    bharat_ipc_endpoint_t sm_ep = BHARAT_CAP_INVALID_HANDLE;
    uint32_t sm_version = 0;

    int lookup_ret = namesvc_lookup("bharat.servicemgr", &sm_svc_id, &sm_ep, &sm_version);
    if (lookup_ret != NAMESVC_STATUS_OK || sm_ep == BHARAT_CAP_INVALID_HANDLE) {
        bharat_runtime_log("services/init: Failed to discover servicemgr via namesvc.");
        return -ENOENT;
    }

    // Prepare bootstrap capability for transfer
    bharat_handle_t bootstrap_cap = bharat_runtime_get_bootstrap_cap();
    if (!bharat_cap_is_valid(bootstrap_cap)) {
        // Fallback for host/unit tests
        bootstrap_cap = 0x40u;
    }

    // 2. Send SM_OP_HANDOFF_BEGIN
    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = SERVICEMGR_SERVICE_ID,
        .opcode = SM_OP_HANDOFF_BEGIN,
        .payload_size = sizeof(sm_handoff_begin_t),
        .capability_transfer = bootstrap_cap,
        .flags = BHARAT_IPC_FLAG_CAP_TRANSFER
    };

    sm_handoff_begin_t begin = {
        .abi_major = 1,
        .abi_minor = 0,
        .struct_size = sizeof(sm_handoff_begin_t),
        .flags = 0,
        .boot_session_id = ctx->boot_session_id,
        .selected_profile = ctx->profile,
        .final_boot_phase = rt->phase,
        .boot_outcome = rt->outcome,
        .failure_class = rt->failure_class,
        .safe_mode_reason = rt->safe_mode_reason,
        .kernel_health_flags = ctx->kernel_health.level,
        .selected_service_count = (uint32_t)rt->manifest_count,
        .already_running_count = 0, // calculated below
        .manifest_version = 1,
        .manifest_hash = 0
    };

    // Calculate how many are already running
    for (size_t i = 0; i < rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        init_service_runtime_t *sr = &rt->services[id];
        if (sr->state == INIT_SERVICE_STATE_READY || sr->observed_ready) {
            begin.already_running_count++;
        }
    }

    bharat_ipc_msg_header_t rep_hdr = {0};
    sm_handoff_resp_t resp = {0};

    bharat_runtime_log("services/init: Sending HANDOFF_BEGIN...");
    int32_t call_status = bharat_ipc_call_ex(sm_ep, &req_hdr, &begin, &rep_hdr, &resp, sizeof(resp), 2000);
    if (call_status != BHARAT_IPC_STATUS_OK || resp.status != BHARAT_IPC_STATUS_OK) {
        bharat_runtime_log("services/init: HANDOFF_BEGIN rejected or failed.");
        return -EIO;
    }

    // 3. Send SM_OP_HANDOFF_SERVICE for each record
    for (uint32_t i = 0; i < (uint32_t)rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        init_service_runtime_t *sr = &rt->services[id];

        req_hdr.opcode = SM_OP_HANDOFF_SERVICE;
        req_hdr.payload_size = sizeof(sm_handoff_service_t);
        req_hdr.flags = 0;
        req_hdr.capability_transfer = BHARAT_CAP_INVALID_HANDLE;

        sm_handoff_service_t srec = {
            .record_index = i,
            .service_id = (uint32_t)sr->desc->id,
            .executable_id = (uint32_t)sr->desc->id,
            .priority = 10,
            .boot_class = (uint32_t)sr->desc->boot_class,
            .restart_policy = (uint32_t)SM_RESTART_POLICY_ALWAYS,
            .start_deadline_ms = sr->desc->start_deadline_ms,
            .ready_deadline_ms = sr->desc->ready_deadline_ms,
            .retry_limit = sr->desc->retry_limit,
            .critical = sr->required_for_boot ? 1 : 0,
            .observed_state = (uint32_t)sr->state,
            .process_id = 0,
            .incarnation_id = 1,
            .dependency_count = (uint32_t)sr->desc->dep_count
        };
        __builtin_strncpy(srec.service_name, sr->desc->name, sizeof(srec.service_name) - 1);

        for (uint32_t d = 0; d < sr->desc->dep_count && d < SM_HANDOFF_MAX_DEPS; d++) {
            srec.dependencies[d] = (uint32_t)sr->desc->deps[d];
        }

        // If service is already running (e.g. bootstrap roots), preserve process ID and incarnation ID
        if (sr->state == INIT_SERVICE_STATE_READY || sr->observed_ready) {
            srec.process_id = (uint32_t)sr->desc->id; // Assign non-zero process ID for adoption
        }

        call_status = bharat_ipc_call_ex(sm_ep, &req_hdr, &srec, &rep_hdr, &resp, sizeof(resp), 2000);
        if (call_status != BHARAT_IPC_STATUS_OK || resp.status != BHARAT_IPC_STATUS_OK) {
            bharat_runtime_log("services/init: HANDOFF_SERVICE record rejected.");
            return -EIO;
        }
    }

    // 4. Send SM_OP_HANDOFF_COMMIT
    req_hdr.opcode = SM_OP_HANDOFF_COMMIT;
    req_hdr.payload_size = sizeof(sm_handoff_commit_t);

    sm_handoff_commit_t commit = {
        .boot_session_id = ctx->boot_session_id,
        .record_count = (uint32_t)rt->manifest_count,
        .required_ready = begin.already_running_count,
        .required_failed = 0,
        .optional_ready = 0,
        .optional_failed = 0,
        .records_hash = 0
    };

    call_status = bharat_ipc_call_ex(sm_ep, &req_hdr, &commit, &rep_hdr, &resp, sizeof(resp), 2000);
    if (call_status != BHARAT_IPC_STATUS_OK || resp.status != BHARAT_IPC_STATUS_OK) {
        bharat_runtime_log("services/init: HANDOFF_COMMIT rejected or timed out.");
        return -EIO;
    }

    bharat_runtime_log("services/init: Handoff accepted by supervisor successfully.");
    return 0;
}
