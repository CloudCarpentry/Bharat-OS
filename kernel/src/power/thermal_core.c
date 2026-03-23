#include "power/thermal.h"
#include <stddef.h>

#define MAX_THERMAL_ZONES 32

static thermal_zone_t *thermal_zones[MAX_THERMAL_ZONES];
static int num_thermal_zones = 0;

int thermal_zone_register(thermal_zone_t *tz) {
    if (!tz || !tz->ops || !tz->ops->read_temp_mc) {
        return -1;
    }

    if (num_thermal_zones >= MAX_THERMAL_ZONES) {
        return -1;
    }

    thermal_zones[num_thermal_zones++] = tz;
    return 0;
}

int thermal_zone_read_temp(thermal_zone_t *tz, int64_t *temp_mc) {
    if (!tz || !temp_mc) return -1;
    return tz->ops->read_temp_mc(tz, temp_mc);
}
