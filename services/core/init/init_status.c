#include "init_status.h"
#include <bharat/runtime/runtime.h>
#include <stddef.h>

static init_service_runtime_t g_service_runtimes[MAX_INIT_SERVICES];

init_service_runtime_t *init_status_get_runtimes(void) {
    return g_service_runtimes;
}

void init_status_report(void) {
    bharat_runtime_log("--- Init Service Status Report ---");
    for (int i = 0; i < MAX_INIT_SERVICES; i++) {
        if (!g_service_runtimes[i].desc) {
            break;
        }

        const char *state_str = "UNKNOWN";
        switch (g_service_runtimes[i].state) {
            case INIT_SERVICE_STOPPED:  state_str = "STOPPED"; break;
            case INIT_SERVICE_STARTING: state_str = "STARTING"; break;
            case INIT_SERVICE_RUNNING:  state_str = "RUNNING"; break;
            case INIT_SERVICE_FAILED:   state_str = "FAILED"; break;
            case INIT_SERVICE_SKIPPED:  state_str = "SKIPPED"; break;
        }

        // Just output a simple formatted string, avoiding rich snprintf for TINY profile constraints
        // Using multiple runtime logs
        bharat_runtime_log(g_service_runtimes[i].desc->name);
        bharat_runtime_log(" : ");
        bharat_runtime_log(state_str);
    }
    bharat_runtime_log("----------------------------------");
}
