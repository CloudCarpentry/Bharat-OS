#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SENSOR_TYPE_WHEEL_SPEED = 0,
    SENSOR_TYPE_IMU,
    SENSOR_TYPE_STEERING_ANGLE,
    SENSOR_TYPE_BRAKE_PRESSURE,
    SENSOR_TYPE_THROTTLE_POSITION,
    SENSOR_TYPE_TEMPERATURE
} sensor_type_t;

typedef struct {
    sensor_type_t type;
    uint32_t sensor_id;
    uint64_t timestamp_ns;
    bool valid;
    bool stale;
    bool plausible;
    float values[8];
    uint8_t value_count;
} sensor_sample_t;
