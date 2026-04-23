#include "init_handoff.h"
#include <bharat/runtime/runtime.h>
#include <errno.h>

int init_send_handoff_to_servicemgr(const init_handoff_summary_t *summary) {
    if (!summary) return -1;
    bharat_runtime_log("services/init: Sending typed handoff to servicemgr...");
    // TODO: implement actual IPC message sending using summary
    return 0;
}

int init_send_handoff_to_faultmgr(const init_handoff_summary_t *summary) {
    if (!summary) return -1;
    // Optional notification to faultmgr
    return 0;
}

bool init_process_handoff_ack(uint32_t boot_session_id, int status_code) {
    (void)boot_session_id;
    (void)status_code;
    return true; // Stub implementation
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
        }
    }

    if (summary.outcome == INIT_BOOT_OUTCOME_SAFE_MODE) {
        bharat_runtime_log("services/init: Handing off in SAFE MODE.");
    } else {
        bharat_runtime_log("services/init: Handing off in NORMAL MODE.");
    }

    init_send_handoff_to_servicemgr(&summary);

    bharat_runtime_log("services/init: Handoff complete.");
    return 0;
#else
    (void)ctx;
    bharat_runtime_log("services/init: Supervisor handoff not enabled. Idling.");
    return -ENOSYS;
#endif
}
