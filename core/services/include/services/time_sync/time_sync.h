#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TIME_QUALITY_UNSYNCED = 0,
    TIME_QUALITY_LOCAL,
    TIME_QUALITY_DISCIPLINED
} time_quality_t;

typedef struct {
    uint64_t monotonic_ns;
    int64_t estimated_drift_ppb;
    time_quality_t quality;
} time_sync_status_t;

int time_sync_get_status(time_sync_status_t* status);
int time_sync_update_reference(uint64_t ref_time_ns, uint64_t local_time_ns);
