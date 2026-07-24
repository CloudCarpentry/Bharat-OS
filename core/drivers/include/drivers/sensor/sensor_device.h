#pragma once
#include "drivers/sensor/sensor_sample.h"

struct sensor_device;
typedef struct sensor_device sensor_device_t;

typedef struct {
    int (*read_sample)(sensor_device_t* dev, sensor_sample_t* out);
    int (*configure)(sensor_device_t* dev, const void* cfg);
    int (*self_test)(sensor_device_t* dev);
} sensor_device_ops_t;

struct sensor_device {
    const char* name;
    uint32_t sensor_id;
    sensor_type_t type;
    const sensor_device_ops_t* ops;
    void* priv;
};

int sensor_core_register(sensor_device_t* dev);
int sensor_core_unregister(sensor_device_t* dev);
sensor_device_t* sensor_core_get(uint32_t sensor_id);
