#include "services/time_sync/time_sync.h"
#include <stddef.h>

static time_sync_status_t g_status = {
    .monotonic_ns = 0,
    .estimated_drift_ppb = 0,
    .quality = TIME_QUALITY_UNSYNCED
};

static uint64_t last_ref_ns = 0;
static uint64_t last_local_ns = 0;

int time_sync_get_status(time_sync_status_t* status) {
    if (!status) return -1;
    *status = g_status;
    return 0;
}

int time_sync_update_reference(uint64_t ref_time_ns, uint64_t local_time_ns) {
    if (last_ref_ns == 0 || last_local_ns == 0) {
        last_ref_ns = ref_time_ns;
        last_local_ns = local_time_ns;
        g_status.quality = TIME_QUALITY_LOCAL; // At least we have a baseline
        return 0;
    }

    // Very simple linear drift estimation (ppb)
    int64_t ref_diff = ref_time_ns - last_ref_ns;
    int64_t loc_diff = local_time_ns - last_local_ns;

    if (loc_diff > 0) {
        int64_t diff = ref_diff - loc_diff;
        // diff per ns * 10^9 = ppb
        g_status.estimated_drift_ppb = (diff * 1000000000LL) / loc_diff;
        g_status.quality = TIME_QUALITY_DISCIPLINED;
    }

    last_ref_ns = ref_time_ns;
    last_local_ns = local_time_ns;

    return 0;
}
