#ifndef BHARAT_OS_COOLING_H
#define BHARAT_OS_COOLING_H

#include "power/thermal.h"

typedef struct cooling_device cooling_device_t;

typedef enum {
    COOLING_TYPE_FAN,
    COOLING_TYPE_CPUFREQ,
    COOLING_TYPE_DEVFREQ,
    COOLING_TYPE_BACKLIGHT,
    COOLING_TYPE_BATTERY,
    COOLING_TYPE_GENERIC
} cooling_type_t;

struct cooling_device_ops {
    int (*get_max_state)(cooling_device_t *cdev, uint32_t *state);
    int (*get_cur_state)(cooling_device_t *cdev, uint32_t *state);
    int (*set_cur_state)(cooling_device_t *cdev, uint32_t state);
};

struct cooling_device {
    int id;
    const char *name;
    cooling_type_t type;
    struct cooling_device_ops *ops;
    void *priv;
};

int cooling_device_register(cooling_device_t *cdev);
int cooling_device_bind_zone(thermal_zone_t *tz, cooling_device_t *cdev, uint32_t trip_id);
int cooling_device_unbind_zone(thermal_zone_t *tz, cooling_device_t *cdev, uint32_t trip_id);

#endif /* BHARAT_OS_COOLING_H */
