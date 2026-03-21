#include <stdio.h>
#include <assert.h>
#include "drivers/sensor/sensor_sample.h"

void sensor_hub_set_time(uint64_t time_ns);
int sensor_hub_check_freshness(sensor_sample_t* sample);

void test_freshness() {
    sensor_hub_set_time(200000000ULL);

    sensor_sample_t sample_fresh = { .timestamp_ns = 150000000ULL }; // 50ms old
    assert(sensor_hub_check_freshness(&sample_fresh) == 0);
    assert(!sample_fresh.stale);

    sensor_sample_t sample_stale = { .timestamp_ns =  50000000ULL }; // 150ms old
    assert(sensor_hub_check_freshness(&sample_stale) == -1);
    assert(sample_stale.stale);
}

int main() {
    test_freshness();
    printf("test_sensor_freshness passed.\n");
    return 0;
}
