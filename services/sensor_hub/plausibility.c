#include "drivers/sensor/sensor_sample.h"
#include <math.h>

#define MAX_WHEEL_SPEED_JUMP 50.0f // max allowed jump in km/h between fast samples

// A simple state keeper for plausibility tests
static float last_wheel_speed[4] = {-1.0f, -1.0f, -1.0f, -1.0f};

void sensor_plausibility_reset(void) {
    for (int i = 0; i < 4; i++) {
        last_wheel_speed[i] = -1.0f;
    }
}

int sensor_hub_check_plausibility(sensor_sample_t* sample) {
    if (!sample) return -1;

    sample->plausible = true;

    if (sample->type == SENSOR_TYPE_WHEEL_SPEED) {
        if (sample->value_count > 0) {
            float speed = sample->values[0];
            uint32_t wheel_idx = sample->sensor_id % 4; // simple mock mapping

            if (last_wheel_speed[wheel_idx] >= 0.0f) {
                float diff = speed - last_wheel_speed[wheel_idx];
                if (diff < 0) diff = -diff;

                if (diff > MAX_WHEEL_SPEED_JUMP) {
                    sample->plausible = false;
                }
            }
            last_wheel_speed[wheel_idx] = speed;
        }
    } else if (sample->type == SENSOR_TYPE_IMU) {
        for (int i = 0; i < sample->value_count; i++) {
            // Check for NaN or infinity
            if (isnan(sample->values[i]) || isinf(sample->values[i])) {
                sample->plausible = false;
                break;
            }
        }
    }

    return sample->plausible ? 0 : -1;
}
