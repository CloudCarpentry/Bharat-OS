#include "init_status.h"
#include "init_manifest.h"
#include <bharat/runtime/runtime.h>

static const char* state_to_str(init_service_state_t state) {
    switch (state) {
        case INIT_SERVICE_STATE_DISABLED: return "DISABLED";
        case INIT_SERVICE_STATE_PENDING: return "PENDING";
        case INIT_SERVICE_STATE_WAITING_DEPS: return "WAITING_DEPS";
        case INIT_SERVICE_STATE_LAUNCH_REQUESTED: return "STARTING";
        case INIT_SERVICE_STATE_REGISTERED: return "REGISTERED";
        case INIT_SERVICE_STATE_READY: return "READY";
        case INIT_SERVICE_STATE_FAILED: return "FAILED";
        case INIT_SERVICE_STATE_SKIPPED: return "SKIPPED";
        default: return "UNKNOWN";
    }
}

void init_status_report(const init_service_runtime_t *runtimes, size_t count) {
#ifdef BHARAT_INIT_ENABLE_STATUS_QUERY
    bharat_runtime_log("--- Init Boot Status Report ---");
    for (size_t i = 0; i < count; i++) {
        const init_service_runtime_t *rt = &runtimes[i];
        if (!rt->desc) continue;

        // Use sequential logging to avoid printf assumptions
        bharat_runtime_log(rt->desc->name);
        bharat_runtime_log(": ");
        bharat_runtime_log(state_to_str(rt->state));

        if (rt->state == INIT_SERVICE_FAILED) {
             bharat_runtime_log(" (Failed)");
        }
    }
    bharat_runtime_log("-------------------------------");
#else
    (void)runtimes;
    (void)count;
#endif
}
