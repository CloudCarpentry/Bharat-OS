#include "power/thermal.h"
#include <stddef.h>

int thermal_zone_get_trips(thermal_zone_t *tz, thermal_trip_desc_t *buf, size_t *count) {
    if (!tz || !buf || !count) return -1;
    if (!tz->ops || !tz->ops->get_trips) return -1;

    return tz->ops->get_trips(tz, buf, count);
}

int thermal_zone_set_trip(thermal_zone_t *tz, const thermal_trip_cfg_t *cfg) {
    if (!tz || !cfg) return -1;
    if (!tz->ops || !tz->ops->set_trip) return -1;

    return tz->ops->set_trip(tz, cfg);
}
