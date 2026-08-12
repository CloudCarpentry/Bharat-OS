#ifndef BHARAT_DRIVERS_VIRTUAL_SENSORS_H
#define BHARAT_DRIVERS_VIRTUAL_SENSORS_H

#include <bharat/uapi/device/sensor.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>

#define BH_VIRTUAL_SENSOR_COUNT 5u

typedef struct bh_virtual_sensor_bank {
    uint64_t timestamp_ns;
    uint32_t sequence;
} bh_virtual_sensor_bank_t;

void bh_virtual_sensors_init(bh_virtual_sensor_bank_t *bank);
bh_status_t bh_virtual_sensor_read(bh_virtual_sensor_bank_t *bank,
                                   bh_sensor_type_t type,
                                   bh_sensor_sample_t *sample);

#endif
