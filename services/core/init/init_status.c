#include "init_status.h"
#include <bharat/runtime/runtime.h>

static const char* state_to_str(init_service_state_t state) {
    switch (state) {
        case INIT_SERVICE_STOPPED: return "STOPPED";
        case INIT_SERVICE_STARTING: return "STARTING";
        case INIT_SERVICE_RUNNING: return "RUNNING";
        case INIT_SERVICE_FAILED: return "FAILED";
        case INIT_SERVICE_SKIPPED: return "SKIPPED";
        default: return "UNKNOWN";
    }
}

void init_status_report(const init_service_runtime_t *runtimes, size_t count) {
#ifdef BHARAT_INIT_ENABLE_STATUS_QUERY
    bharat_runtime_log("--- Init Boot Status Report ---");
    for (size_t i = 0; i < count; i++) {
        const init_service_runtime_t *rt = &runtimes[i];

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
