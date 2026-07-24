#include <stdio.h>
#include <assert.h>
#include "services/time_sync/time_sync.h"

// Internal functions
int time_sync_get_status(time_sync_status_t* status);
int time_sync_update_reference(uint64_t ref_time_ns, uint64_t local_time_ns);

void test_drift_calculation() {
    time_sync_status_t status;
    assert(time_sync_get_status(&status) == 0);
    assert(status.quality == TIME_QUALITY_UNSYNCED);

    // First reference sync
    assert(time_sync_update_reference(1000000000ULL, 1000000000ULL) == 0);
    assert(time_sync_get_status(&status) == 0);
    // Becomes local/baseline

    // Second reference sync, ref time moved exactly 1s, local moved exactly 1s
    assert(time_sync_update_reference(2000000000ULL, 2000000000ULL) == 0);
    assert(time_sync_get_status(&status) == 0);
    assert(status.quality == TIME_QUALITY_DISCIPLINED);
    assert(status.estimated_drift_ppb == 0);

    // Third sync, local time is slightly slower than reference
    // Ref time moved 1s, local time moved 0.999s
    assert(time_sync_update_reference(3000000000ULL, 2999000000ULL) == 0);
    assert(time_sync_get_status(&status) == 0);
    // Estimated drift should be positive (~1000000 ppb)
    // Diff between ref move and local move is 1000000 ns
    assert(status.estimated_drift_ppb > 0);
}

int main() {
    test_drift_calculation();
    printf("test_time_sync_drift passed.\n");
    return 0;
}
