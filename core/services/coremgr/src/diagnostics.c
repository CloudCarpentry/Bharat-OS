#include <bharat/coremgr/coremgr.h>
#include <stddef.h>

uint32_t g_invalid_events = 0;
uint32_t g_registry_overflows = 0;

coremgr_diagnostics_t coremgr_get_diagnostics(void) {
    coremgr_diagnostics_t diag = {0};
    diag.invalid_events = g_invalid_events;
    diag.registry_overflows = g_registry_overflows;
    diag.transport_status = CORE_TRANSPORT_HEALTHY;

    for (uint32_t i = 0; i < COREMGR_MAX_CORES; i++) {
        coremgr_core_info_t* info = coremgr_get_core_info_by_index(i);
        if (info && info->core_id != (uint32_t)-1) {
            diag.total_detected++;
            if (info->is_online) {
                diag.online_count++;
            }
            if (info->lifecycle_state == CORE_STATE_DEGRADED) {
                diag.degraded_count++;
            }
            if (info->lifecycle_state == CORE_STATE_FAILED) {
                diag.failed_count++;
            }
            if (info->transport_state != CORE_TRANSPORT_HEALTHY) {
                diag.transport_status = CORE_TRANSPORT_DEGRADED;
            }
        }
    }

    return diag;
}

void coremgr_inc_invalid_event(void) {
    g_invalid_events++;
}

void coremgr_inc_registry_overflow(void) {
    g_registry_overflows++;
}
