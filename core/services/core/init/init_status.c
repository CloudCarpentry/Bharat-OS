#include "init_status.h"
#include "init_manifest.h"
#include <bharat/runtime/runtime.h>

static const char* state_to_str(init_service_state_t state) {
    switch (state) {
        case INIT_SERVICE_STATE_DISABLED: return "DISABLED";
        case INIT_SERVICE_STATE_DECLARED: return "DECLARED";
        case INIT_SERVICE_STATE_WAITING_DEPS: return "WAITING_DEPS";
        case INIT_SERVICE_STATE_SPAWN_REQUESTED: return "SPAWN_REQUESTED";
        case INIT_SERVICE_STATE_SPAWNED: return "SPAWNED";
        case INIT_SERVICE_STATE_ENDPOINT_BOUND: return "ENDPOINT_BOUND";
        case INIT_SERVICE_STATE_READY: return "READY";
        case INIT_SERVICE_STATE_FAILED: return "FAILED";
        case INIT_SERVICE_STATE_SKIPPED: return "SKIPPED";
        default: return "UNKNOWN";
    }
}

void init_status_report(const init_service_runtime_t *runtimes, size_t count) {
#ifdef BHARAT_INIT_ENABLE_STATUS_QUERY
    bharat_runtime_log("--- Init Boot Status Report ---\n");
    for (size_t i = 0; i < count; i++) {
        const init_service_runtime_t *rt = &runtimes[i];
        if (!rt->desc) continue;

        // Use sequential logging to avoid printf assumptions
        bharat_runtime_log(rt->desc->name);
        bharat_runtime_log(": ");
        bharat_runtime_log(state_to_str(rt->state));
        if (rt->last_error) {
            char error[] = " error=0x00000000\n";
            uint32_t value = (uint32_t)rt->last_error;
            for (unsigned j = 0; j < 8; ++j) {
                unsigned digit = (value >> ((7 - j) * 4)) & 15;
                error[9 + j] = "0123456789abcdef"[digit];
            }
            bharat_runtime_log(error);
        } else {
            bharat_runtime_log("\n");
        }

        if (rt->state == INIT_SERVICE_STATE_FAILED) {
             bharat_runtime_log(" (Failed)\n");
        }
    }
    bharat_runtime_log("-------------------------------\n");
#else
    (void)runtimes;
    (void)count;
#endif
}
