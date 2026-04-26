#include "init_handoff.h"
#include <bharat/runtime/runtime.h>
#include <errno.h>

int init_send_handoff_to_servicemgr(const init_handoff_summary_t *summary) {
    if (!summary) return -1;
    bharat_runtime_log("services/init: Sending typed handoff to servicemgr...");
    // TODO: implement actual IPC message sending using summary

    // Mock successful send for now
    return 0;
}

int init_send_handoff_to_faultmgr(const init_handoff_summary_t *summary) {
    if (!summary) return -1;
    bharat_runtime_log("services/init: Sending typed handoff to faultmgr...");
    // Optional notification to faultmgr
    return 0;
}

bool init_process_handoff_ack(uint32_t boot_session_id, int status_code) {
    (void)boot_session_id;
    (void)status_code;

    bharat_runtime_log("services/init: Received handoff ACK.");

    if (status_code != 0) {
        bharat_runtime_log("services/init: Handoff was REJECTED by supervisor.");
        return false;
    }

    return true;
}

int init_handoff_to_supervisor(const init_boot_context_t *ctx) {
#ifdef BHARAT_INIT_ENABLE_HANDOFF
    bharat_runtime_log("services/init: Preparing handoff to supervisor...");

    init_handoff_summary_t summary = {0};
    if (ctx) {
        summary.boot_session_id = ctx->boot_session_id;
        summary.profile = ctx->profile;
        if (ctx->safe_mode_requested) {
            summary.outcome = INIT_BOOT_OUTCOME_SAFE_MODE;
        } else {
            summary.outcome = INIT_BOOT_OUTCOME_SUCCESS;
        }
    }

    if (summary.outcome == INIT_BOOT_OUTCOME_SAFE_MODE) {
        bharat_runtime_log("services/init: Handing off in SAFE MODE.");
    } else {
        bharat_runtime_log("services/init: Handing off in NORMAL MODE.");
    }

    int res = init_send_handoff_to_servicemgr(&summary);
    if (res != 0) {
        bharat_runtime_log("services/init: Handoff send failed.");
        return -EIO;
    }

    // In a real implementation we would wait for ACK with timeout here.
    // For this stub, we simulate a successful ACK.
    bool acked = init_process_handoff_ack(summary.boot_session_id, 0);
    if (!acked) {
        bharat_runtime_log("services/init: Handoff timeout or rejection.");
        return -ETIMEDOUT;
    }

    bharat_runtime_log("services/init: Handoff complete.");
    return 0;
#else
    (void)ctx;
    bharat_runtime_log("services/init: Supervisor handoff not enabled. Idling.");
    return -ENOSYS;
#endif
}
