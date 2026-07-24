#ifndef BHARAT_UAPI_WATCHDOG_H
#define BHARAT_UAPI_WATCHDOG_H

#include <stdint.h>

typedef enum bh_watchdog_status {
    BH_WATCHDOG_OK = 0,
    BH_WATCHDOG_LATE,
    BH_WATCHDOG_MISSED,
    BH_WATCHDOG_EXPIRED,
} bh_watchdog_status_t;

typedef struct bh_watchdog_heartbeat {
    uint32_t task_id;
    uint64_t period_ns;
    uint64_t last_seen_ns;
    uint32_t missed_count;
} bh_watchdog_heartbeat_t;

#endif // BHARAT_UAPI_WATCHDOG_H
