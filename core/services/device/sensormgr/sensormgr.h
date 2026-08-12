#ifndef BHARAT_SERVICES_SENSORMGR_H
#define BHARAT_SERVICES_SENSORMGR_H

#include <bharat/uapi/device/sensor.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>
#include "../../../drivers/sensor/virtual/virtual_sensors.h"

#define BH_SENSORMGR_MAX_STREAMS 8u
#define BH_SENSORMGR_MIN_RATE_HZ 1u
#define BH_SENSORMGR_MAX_RATE_HZ 1000u

typedef struct bh_sensormgr_stream_slot {
    bh_sensor_type_t type;
    uint32_t rate_hz;
    uint16_t generation;
    uint8_t active;
} bh_sensormgr_stream_slot_t;

/* A service instance owns this table; callers must serialize access to it. */
typedef struct bh_sensormgr {
    bh_virtual_sensor_bank_t *devices;
    bh_sensormgr_stream_slot_t streams[BH_SENSORMGR_MAX_STREAMS];
} bh_sensormgr_t;

void bh_sensormgr_init(bh_sensormgr_t *manager, bh_virtual_sensor_bank_t *devices);
bh_status_t bh_sensormgr_open(bh_sensormgr_t *manager,
                           bh_sensor_type_t type,
                           bh_sensor_handle_t *sensor);
bh_status_t bh_sensormgr_subscribe(bh_sensormgr_t *manager,
                                bh_sensor_handle_t sensor,
                                uint32_t rate_hz,
                                bh_sensor_stream_t *stream);
bh_status_t bh_sensormgr_read(bh_sensormgr_t *manager,
                           bh_sensor_stream_t stream,
                           bh_sensor_sample_t *sample);
bh_status_t bh_sensormgr_unsubscribe(bh_sensormgr_t *manager, bh_sensor_stream_t stream);

#endif
