#include <bharat/kernel/power/power.h>
#include "console/console_core.h"
#include "lib/base/string.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

void bh_power_signal_event(uint32_t event_type) {
    (void)event_type;
    // Minimal primitive hook
    KPRINT("POWER EVENT\n");
}

void bh_thermal_signal_trip(uint32_t zone_id, int32_t temp) {
    (void)zone_id; (void)temp;
    // Minimal primitive hook
    KPRINT("THERMAL TRIP\n");
}
