#include "power/thermal.h"
#include <stddef.h>

/* Null thermal implementation provides dummy generic ops */

static int null_tz_read_temp(thermal_zone_t *tz, int64_t *temp_mc) {
    if (!tz || !temp_mc) return -1;
    *temp_mc = 25000; /* Fixed at 25°C */
    return 0;
}

static int null_tz_get_trips(thermal_zone_t *tz, thermal_trip_desc_t *buf, size_t *count) {
    if (!tz || !count) return -1;
    *count = 0; /* No trip points */
    return 0;
}

static int null_tz_set_trip(thermal_zone_t *tz, const thermal_trip_cfg_t *cfg) {
    if (!tz || !cfg) return -1;
    return -1; /* Setting trip points is not supported */
}

struct thermal_zone_ops null_thermal_zone_ops = {
    .read_temp_mc = null_tz_read_temp,
    .get_trips = null_tz_get_trips,
    .set_trip = null_tz_set_trip
};
