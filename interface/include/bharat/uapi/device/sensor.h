#ifndef BHARAT_UAPI_DEVICE_SENSOR_H
#define BHARAT_UAPI_DEVICE_SENSOR_H

#include <stddef.h>
#include <stdint.h>

#define BH_SENSOR_ABI_VERSION 1u
#define BH_SENSOR_MAX_VALUES 4u

typedef uint32_t bh_sensor_id_t;
typedef uint32_t bh_sensor_handle_t;
typedef uint32_t bh_sensor_stream_t;

typedef enum bh_sensor_type {
    BH_SENSOR_IMU = 1,
    BH_SENSOR_GPS = 2,
    BH_SENSOR_TEMPERATURE = 3,
    BH_SENSOR_BATTERY = 4,
    BH_SENSOR_DISTANCE = 5
} bh_sensor_type_t;

enum {
    BH_SENSOR_SAMPLE_VALID = 1u << 0,
    BH_SENSOR_SAMPLE_STALE = 1u << 1,
    BH_SENSOR_SAMPLE_FAULT = 1u << 2
};

typedef struct bh_sensor_sample {
    bh_sensor_id_t sensor;
    uint32_t type;
    uint64_t timestamp_ns;
    uint32_t flags;
    uint32_t value_count;
    float values[BH_SENSOR_MAX_VALUES];
} bh_sensor_sample_t;

_Static_assert(sizeof(bh_sensor_sample_t) == 40u, "sensor sample ABI size");
_Static_assert(offsetof(bh_sensor_sample_t, timestamp_ns) == 8u, "sensor timestamp ABI offset");
_Static_assert(offsetof(bh_sensor_sample_t, values) == 24u, "sensor values ABI offset");

#endif
