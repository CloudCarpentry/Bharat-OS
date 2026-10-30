#include "init_handoff.h"
#include "init_contract.h"
#include <bharat/runtime/runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/ipc/ipc.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/uapi/servicemgr/handoff.h>
#include <bharat/uapi/ipc/status.h>
#include <errno.h>

int init_handoff_to_supervisor(const init_boot_context_t *ctx, struct init_runtime_s *rt) {
    if (!ctx || !rt) return -EINVAL;

    bharat_ipc_endpoint_t sm_ep = rt->services[INIT_SVC_SERVICEMGR].launch.service_send_cap;
    if (sm_ep == 0U || sm_ep == BHARAT_CAP_INVALID_HANDLE) {
        bharat_service_id_t svc_id = 0;
        uint32_t svc_ver = 0;
        int lookup_rc = namesvc_lookup("bharat.servicemgr", &svc_id, &sm_ep, &svc_ver);
        if (lookup_rc != NAMESVC_STATUS_OK || sm_ep == 0U || sm_ep == BHARAT_CAP_INVALID_HANDLE) {
            return -ENOENT;
        }
    }

    uint32_t auth_cap = bharat_runtime_get_bootstrap_cap();
    if (auth_cap == 0U || auth_cap == BHARAT_CAP_INVALID_HANDLE) {
        bharat_runtime_log("services/init: supervisor handoff denied: missing bootstrap authority\n");
        return -EPERM;
    }

    uint32_t record_count = 0U;
    uint32_t running_count = 0U;
    uint32_t req_ready = 0U, req_failed = 0U, opt_ready = 0U, opt_failed = 0U;

    for (size_t i = 0; i < rt->manifest_count; ++i) {
        init_service_id_t id = rt->service_order[i];
        const init_service_runtime_t *sr = &rt->services[id];
        if (!sr->desc || sr->state == INIT_SERVICE_STATE_SKIPPED || sr->state == INIT_SERVICE_STATE_DISABLED) {
            continue;
        }
        record_count++;
        if (sr->state == INIT_SERVICE_STATE_READY) {
            running_count++;
            if (sr->required_for_boot) req_ready++;
            else opt_ready++;
        } else if (sr->state == INIT_SERVICE_STATE_FAILED) {
            if (sr->required_for_boot) req_failed++;
            else opt_failed++;
        }
    }

    uint64_t session_id = ctx->boot_session_id ? (uint64_t)ctx->boot_session_id : 1ULL;
    sm_handoff_begin_t begin_req = {
        .abi_major = 1U,
        .abi_minor = 0U,
        .struct_size = sizeof(sm_handoff_begin_t),
        .flags = 0U,
        .boot_session_id = session_id,
        .selected_profile = (uint32_t)ctx->profile,
        .final_boot_phase = (uint32_t)rt->phase,
        .boot_outcome = (uint32_t)rt->outcome,
        .failure_class = (uint32_t)rt->failure_class,
        .safe_mode_reason = rt->safe_mode_reason,
        .kernel_health_flags = ctx->kernel_health.degraded_feature_mask,
        .selected_service_count = record_count,
        .already_running_count = running_count,
        .manifest_version = 1U,
        .manifest_hash = 0ULL,
    };

    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = SERVICEMGR_SERVICE_ID,
        .opcode = SM_OP_HANDOFF_BEGIN,
        .payload_size = sizeof(begin_req),
        .capability_transfer = auth_cap,
    };
    bharat_ipc_msg_header_t rep_hdr = {0};
    sm_handoff_resp_t rep = {0};

    int32_t rc = bharat_ipc_call_ex(sm_ep, &req_hdr, &begin_req, &rep_hdr, &rep, sizeof(rep), 1000U);
    if (rc != BHARAT_IPC_STATUS_OK || rep.status != BHARAT_IPC_STATUS_OK) {
        return (rc == BHARAT_IPC_STATUS_ERR_PERM || rep.status == BHARAT_IPC_STATUS_ERR_PERM) ? -EPERM : -EIO;
    }

    uint16_t rec_idx = 0U;
    for (size_t i = 0; i < rt->manifest_count; ++i) {
        init_service_id_t id = rt->service_order[i];
        const init_service_runtime_t *sr = &rt->services[id];
        if (!sr->desc || sr->state == INIT_SERVICE_STATE_SKIPPED || sr->state == INIT_SERVICE_STATE_DISABLED) {
            continue;
        }

        sm_handoff_service_t svc_req = {0};
        svc_req.incarnation_id = sr->attempts ? (uint64_t)sr->attempts : 1ULL;
        svc_req.process_id = (uint32_t)sr->launch.process_id;
        svc_req.record_index = rec_idx++;
        svc_req.service_id = (uint16_t)sr->desc->id;
        svc_req.executable_id = (uint16_t)sr->desc->executable_desc.executable_handle;
        svc_req.start_deadline_ms = (uint16_t)sr->desc->start_deadline_ms;
        svc_req.ready_deadline_ms = (uint16_t)sr->desc->ready_deadline_ms;
        svc_req.priority = 10U;
        svc_req.boot_class = (uint8_t)sr->desc->boot_class;
        svc_req.restart_policy = sr->desc->retry_limit > 0U
            ? (uint8_t)SM_RESTART_POLICY_BOUNDED_RETRY
            : (uint8_t)SM_RESTART_POLICY_NONE;
        svc_req.retry_limit = sr->desc->retry_limit;
        svc_req.critical = sr->required_for_boot ? 1U : 0U;
        svc_req.observed_state = (uint8_t)sr->state;

        uint8_t dep_cnt = 0U;
        for (uint8_t d = 0U; d < sr->desc->dep_count && dep_cnt < SM_HANDOFF_MAX_DEPS; ++d) {
            init_service_id_t dep_id = sr->desc->deps[d];
            if (dep_id > INIT_SVC_NONE && dep_id < INIT_SERVICE_ID_MAX &&
                rt->services[dep_id].desc != NULL &&
                rt->services[dep_id].state != INIT_SERVICE_STATE_SKIPPED &&
                rt->services[dep_id].state != INIT_SERVICE_STATE_DISABLED) {
                svc_req.dependencies[dep_cnt++] = (uint8_t)dep_id;
            }
        }
        svc_req.dependency_count = dep_cnt;
        if (sr->desc->name) {
            for (size_t c = 0; c < sizeof(svc_req.service_name) - 1U && sr->desc->name[c]; ++c) {
                svc_req.service_name[c] = sr->desc->name[c];
            }
        }

        req_hdr.opcode = SM_OP_HANDOFF_SERVICE;
        req_hdr.payload_size = sizeof(svc_req);
        req_hdr.capability_transfer = auth_cap;
        rep = (sm_handoff_resp_t){0};

        rc = bharat_ipc_call_ex(sm_ep, &req_hdr, &svc_req, &rep_hdr, &rep, sizeof(rep), 1000U);
        if (rc != BHARAT_IPC_STATUS_OK || rep.status != BHARAT_IPC_STATUS_OK) {
            return (rc == BHARAT_IPC_STATUS_ERR_PERM || rep.status == BHARAT_IPC_STATUS_ERR_PERM) ? -EPERM : -EIO;
        }
    }

    sm_handoff_commit_t commit_req = {
        .boot_session_id = session_id,
        .record_count = record_count,
        .required_ready = req_ready,
        .required_failed = req_failed,
        .optional_ready = opt_ready,
        .optional_failed = opt_failed,
        .records_hash = 0ULL,
    };
    req_hdr.opcode = SM_OP_HANDOFF_COMMIT;
    req_hdr.payload_size = sizeof(commit_req);
    req_hdr.capability_transfer = auth_cap;
    rep = (sm_handoff_resp_t){0};

    rc = bharat_ipc_call_ex(sm_ep, &req_hdr, &commit_req, &rep_hdr, &rep, sizeof(rep), 1000U);
    if (rc != BHARAT_IPC_STATUS_OK || rep.status != BHARAT_IPC_STATUS_OK) {
        return (rc == BHARAT_IPC_STATUS_ERR_PERM || rep.status == BHARAT_IPC_STATUS_ERR_PERM) ? -EPERM : -EIO;
    }

    rt->handoff_ack_received = true;
    bharat_runtime_log("BOOT_HANDOFF_COMMITTED\n");
    return 0;
}
