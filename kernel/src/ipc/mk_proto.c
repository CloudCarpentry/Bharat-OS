#include "../../include/ipc/mk_proto.h"

// Minimal, non-invasive L1 policy helpers. These do not change runtime
// behavior yet; they provide a single place to encode retry/idempotency
// rules and map outcomes to transaction states.

static uint32_t mk_proto_policy_flags_for(uint32_t msg_type) {
    switch (msg_type) {
        // Read-only / lookup style operations → idempotent + retryable
        case MK_MSG_THREAD_LOOKUP_REQ:
        case MK_MSG_PROCESS_LOOKUP_REQ:
        case MK_MSG_CAP_LOOKUP_REQ:
            return MK_PROTO_POLICY_ACK_REQUIRED |
                   MK_PROTO_POLICY_IDEMPOTENT |
                   MK_PROTO_POLICY_RETRYABLE;

        // State-changing operations → no automatic retry
        case MK_MSG_THREAD_HANDOFF_REQ:
        case MK_MSG_FRAME_ALLOC_REQ:
        case MK_MSG_FRAME_FREE_REQ:
        case MK_MSG_FRAME_MAP_REQ:
        case MK_MSG_FRAME_UNMAP_REQ:
        case MK_MSG_CAP_GRANT_REQ:
        case MK_MSG_CAP_REVOKE_REQ:
            return MK_PROTO_POLICY_ACK_REQUIRED |
                   MK_PROTO_POLICY_STATE_MUTATION;

        default:
            // Conservative default
            return MK_PROTO_POLICY_ACK_REQUIRED;
    }
}

int mk_proto_get_policy(uint32_t msg_type, mk_proto_policy_t *out_policy) {
    if (!out_policy) {
        return -1;
    }
    out_policy->msg_type = msg_type;
    out_policy->flags = mk_proto_policy_flags_for(msg_type);
    out_policy->max_retries = (out_policy->flags & MK_PROTO_POLICY_RETRYABLE) ? 2U : 0U;
    return 0;
}

int mk_proto_is_idempotent(uint32_t msg_type) {
    return (mk_proto_policy_flags_for(msg_type) & MK_PROTO_POLICY_IDEMPOTENT) != 0U;
}

int mk_proto_should_retry(uint32_t msg_type, mk_proto_result_t result,
                          uint32_t retry_count) {
    uint32_t flags = mk_proto_policy_flags_for(msg_type);

    if (!(flags & MK_PROTO_POLICY_RETRYABLE)) {
        return 0;
    }

    if (result == MK_PROTO_RESULT_TIMEOUT || result == MK_PROTO_RESULT_DUPLICATE) {
        // Retry bounded times only
        if (retry_count < 2U) {
            return 1;
        }
    }

    return 0;
}

mk_txn_state_t mk_proto_result_to_txn_state(mk_proto_result_t result) {
    switch (result) {
        case MK_PROTO_RESULT_OK:
            return MK_TXN_STATE_ACKED;
        case MK_PROTO_RESULT_TIMEOUT:
            return MK_TXN_STATE_TIMED_OUT;
        case MK_PROTO_RESULT_STALE_ENDPOINT:
        case MK_PROTO_RESULT_CAP_REVOKED:
        case MK_PROTO_RESULT_BAD_AUTH:
        case MK_PROTO_RESULT_BAD_ROUTE:
        case MK_PROTO_RESULT_BAD_PAYLOAD:
        case MK_PROTO_RESULT_UNSUPPORTED:
        case MK_PROTO_RESULT_RETRY_NOT_ALLOWED:
            return MK_TXN_STATE_CANCELLED;
        case MK_PROTO_RESULT_DUPLICATE:
            return MK_TXN_STATE_REPLIED;
        default:
            return MK_TXN_STATE_CANCELLED;
    }
}

uint32_t mk_proto_result_to_reason_code(mk_proto_result_t result) {
    switch (result) {
        case MK_PROTO_RESULT_OK: return MK_REASON_SUCCESS;
        case MK_PROTO_RESULT_STALE_ENDPOINT: return MK_REASON_STALE_ENDPOINT;
        case MK_PROTO_RESULT_CAP_REVOKED: return MK_REASON_CAP_REVOKED;
        case MK_PROTO_RESULT_DUPLICATE: return MK_REASON_DUPLICATE;
        case MK_PROTO_RESULT_TIMEOUT: return MK_REASON_TIMEOUT;
        case MK_PROTO_RESULT_BAD_AUTH: return MK_REASON_BAD_AUTH;
        case MK_PROTO_RESULT_BAD_ROUTE: return MK_REASON_BAD_ROUTE;
        case MK_PROTO_RESULT_BAD_PAYLOAD: return MK_REASON_BAD_PAYLOAD;
        case MK_PROTO_RESULT_UNSUPPORTED: return MK_REASON_UNSUPPORTED;
        case MK_PROTO_RESULT_RETRY_NOT_ALLOWED: return MK_REASON_RETRY_NOT_ALLOWED;
        default: return MK_REASON_UNSUPPORTED;
    }
}
