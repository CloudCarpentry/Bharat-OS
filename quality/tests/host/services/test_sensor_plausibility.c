#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "drivers/sensor/sensor_sample.h"

void sensor_plausibility_reset(void);
int sensor_hub_check_plausibility(sensor_sample_t* sample);

void test_wheel_speed_jump() {
    sensor_plausibility_reset();

    sensor_sample_t sample1 = { .type = SENSOR_TYPE_WHEEL_SPEED, .sensor_id = 1, .value_count = 1, .values = {10.0f} };
    assert(sensor_hub_check_plausibility(&sample1) == 0);
    assert(sample1.plausible);

    sensor_sample_t sample2 = { .type = SENSOR_TYPE_WHEEL_SPEED, .sensor_id = 1, .value_count = 1, .values = {15.0f} }; // valid diff
    assert(sensor_hub_check_plausibility(&sample2) == 0);
    assert(sample2.plausible);

    sensor_sample_t sample_bad = { .type = SENSOR_TYPE_WHEEL_SPEED, .sensor_id = 1, .value_count = 1, .values = {100.0f} }; // >50 diff
    assert(sensor_hub_check_plausibility(&sample_bad) == -1);
    assert(!sample_bad.plausible);
}

void test_imu_nan() {
    sensor_sample_t sample_imu = { .type = SENSOR_TYPE_IMU, .value_count = 3, .values = {0.0f, 1.0f, NAN} };
    assert(sensor_hub_check_plausibility(&sample_imu) == -1);
    assert(!sample_imu.plausible);
}

int main() {
    test_wheel_speed_jump();
    test_imu_nan();
    printf("test_sensor_plausibility passed.\n");
    return 0;
}
