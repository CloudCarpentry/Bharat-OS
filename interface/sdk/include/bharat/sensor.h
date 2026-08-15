#ifndef BHARAT_SDK_SENSOR_H
#define BHARAT_SDK_SENSOR_H

#include <bharat/types.h>
#include <bharat/uapi/device/sensor.h>

enum { BH_DEVICE_CLASS_SENSOR = 0x00000020u };

bh_status_t bh_sensor_open(bh_sensor_type_t type, bh_sensor_handle_t *sensor);
bh_status_t bh_sensor_subscribe(bh_sensor_handle_t sensor,
                                uint32_t rate_hz,
                                bh_sensor_stream_t *stream);
bh_status_t bh_sensor_read(bh_sensor_stream_t stream, bh_sensor_sample_t *sample);
bh_status_t bh_sensor_unsubscribe(bh_sensor_stream_t stream);

#endif
