#include "drivers/sensor/sensor_sample.h"

// Suppose system updates a simulated current time
static uint64_t g_current_time_ns = 0;

void sensor_hub_set_time(uint64_t time_ns) {
    g_current_time_ns = time_ns;
}

// 100 ms timeout
#define STALENESS_TIMEOUT_NS 100000000ULL

int sensor_hub_check_freshness(sensor_sample_t* sample) {
    if (!sample) return -1;

    if (g_current_time_ns > sample->timestamp_ns) {
        uint64_t diff = g_current_time_ns - sample->timestamp_ns;
        if (diff > STALENESS_TIMEOUT_NS) {
            sample->stale = true;
            return -1;
        }
    }

    sample->stale = false;
    return 0;
}
