#ifndef BHARAT_SDK_SENSOR_H
#define BHARAT_SDK_SENSOR_H
#include <bharat/device.h>
enum { BH_DEVICE_CLASS_SENSOR = 0x00000020u };
typedef struct bh_sensor_sample { uint64_t timestamp_ns; int32_t values[4]; uint32_t value_count; uint32_t reserved; } bh_sensor_sample_t;
#endif
