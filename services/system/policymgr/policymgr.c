#include "policymgr.h"
#include <bharat/ipc/ipc.h>
#include <stddef.h>

static bharat_resolved_policy_t g_resolved_policy;

void policymgr_init(void) {
    g_resolved_policy.version = 1;
#if defined(BHARAT_PROFILE_MPU_ONLY)
    g_resolved_policy.system_policy.active_tier = BHARAT_POLICY_TIER_A_STATIC;
#elif defined(BHARAT_PROFILE_MMU_LITE)
    g_resolved_policy.system_policy.active_tier = BHARAT_POLICY_TIER_B_LIGHTWEIGHT;
#else
    g_resolved_policy.system_policy.active_tier = BHARAT_POLICY_TIER_C_DYNAMIC;
#endif

    g_resolved_policy.system_policy.allowed_subsystems_mask = 0xFFFFFFFF; // all for now
    g_resolved_policy.system_policy.initial_power_state = BHARAT_POWER_STATE_FULL;
    g_resolved_policy.system_policy.watchdog_timeout_ms = 10000;

    // Define some sane default SLOs based on tier
    if (g_resolved_policy.system_policy.active_tier == BHARAT_POLICY_TIER_A_STATIC) {
        g_resolved_policy.slo_gates.max_dispatch_latency_us = 500;
        g_resolved_policy.slo_gates.max_runqueue_depth = 16;
        g_resolved_policy.slo_gates.max_ipc_queue_depth = 32;
        g_resolved_policy.slo_gates.mem_pressure_threshold_pct = 95;
    } else {
        g_resolved_policy.slo_gates.max_dispatch_latency_us = 1000;
        g_resolved_policy.slo_gates.max_runqueue_depth = 128;
        g_resolved_policy.slo_gates.max_ipc_queue_depth = 256;
        g_resolved_policy.slo_gates.mem_pressure_threshold_pct = 90;
    }
}

void policymgr_run(void) {
    if (g_resolved_policy.system_policy.active_tier == BHARAT_POLICY_TIER_A_STATIC) {
        return; // Tier A exits after initializing resolved policy
    }

    bharat_ipc_msg_header_t req_header;
    bharat_ipc_msg_header_t resp_header;
    uint8_t payload_buf[512];
    uint8_t resp_payload_buf[512];

    bharat_ipc_endpoint_t endpoint = 12; // Example static policymgr ID
    while (1) {
        int32_t recv_status = bharat_ipc_recv(endpoint, &req_header, payload_buf, sizeof(payload_buf));
        if (recv_status < 0) continue;

        if (req_header.interface_id != BHARAT_POLICYMGR_INTERFACE_ID) continue;

        int32_t dispatch_status = BHARAT_IPC_STATUS_ERR_OPCODE;
        uint32_t resp_size = 0;

        resp_header = req_header;

        switch (req_header.opcode) {
            case POLICY_OP_QUERY_SERVICE: {
                if (req_header.payload_size >= sizeof(policy_req_query_service_t)) {
                    policy_req_query_service_t *req = (policy_req_query_service_t*)payload_buf;
                    policy_resp_query_service_t *resp = (policy_resp_query_service_t*)resp_payload_buf;

                    // Deny service 5 as a mock rule to show gating works
                    if (req->service_id == 5) {
                        resp->decision = POLICY_DECISION_DENY;
                        resp->allowed_caps = 0;
                    } else {
                        resp->decision = POLICY_DECISION_ALLOW;
                        resp->allowed_caps = req->requested_caps;
                    }
                    resp->max_restarts = 3;
                    resp->policy_version = g_resolved_policy.version;

                    dispatch_status = BHARAT_IPC_STATUS_OK;
                    resp_size = sizeof(policy_resp_query_service_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            case POLICY_OP_QUERY_SCHED_CONSTRAINTS: {
                if (req_header.payload_size >= sizeof(policy_req_query_sched_t)) {
                    policy_resp_query_sched_t *resp = (policy_resp_query_sched_t*)resp_payload_buf;
                    resp->slo_gates = g_resolved_policy.slo_gates;
                    resp->policy_version = g_resolved_policy.version;
                    resp->default_deadline_ms = 100;

                    dispatch_status = BHARAT_IPC_STATUS_OK;
                    resp_size = sizeof(policy_resp_query_sched_t);
                } else {
                    dispatch_status = BHARAT_IPC_STATUS_ERR_LENGTH;
                }
                break;
            }
            default:
                break;
        }

        resp_header.payload_size = resp_size;
        resp_header.flags = dispatch_status;

        if (req_header.reply_endpoint != 0) {
            bharat_ipc_send(req_header.reply_endpoint, &resp_header, resp_payload_buf);
        }
    }
}
