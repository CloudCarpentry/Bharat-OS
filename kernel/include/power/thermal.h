#ifndef BHARAT_OS_THERMAL_H
#define BHARAT_OS_THERMAL_H

#include <stdint.h>
#include <stddef.h>

typedef struct thermal_zone thermal_zone_t;

typedef enum {
    THERMAL_TRIP_ACTIVE,
    THERMAL_TRIP_PASSIVE,
    THERMAL_TRIP_HOT,
    THERMAL_TRIP_CRITICAL
} thermal_trip_type_t;

typedef struct thermal_trip_cfg {
    int64_t temp_mc;
    int64_t hysteresis_mc;
    thermal_trip_type_t type;
} thermal_trip_cfg_t;

typedef struct thermal_trip_desc {
    int id;
    thermal_trip_cfg_t cfg;
} thermal_trip_desc_t;

struct thermal_zone_ops {
    int (*read_temp_mc)(thermal_zone_t *tz, int64_t *temp_mc);
    int (*get_trips)(thermal_zone_t *tz, thermal_trip_desc_t *buf, size_t *count);
    int (*set_trip)(thermal_zone_t *tz, const thermal_trip_cfg_t *cfg);
};

struct thermal_zone {
    int id;
    const char *name;
    struct thermal_zone_ops *ops;
    void *priv;
};

int thermal_zone_register(thermal_zone_t *tz);
int thermal_zone_read_temp(thermal_zone_t *tz, int64_t *temp_mc);
int thermal_zone_get_trips(thermal_zone_t *tz, thermal_trip_desc_t *buf, size_t *count);
int thermal_zone_set_trip(thermal_zone_t *tz, const thermal_trip_cfg_t *cfg);

#endif /* BHARAT_OS_THERMAL_H */
