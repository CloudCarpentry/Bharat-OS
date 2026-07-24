#ifndef BHARAT_OS_BATTERY_EVENTS_H
#define BHARAT_OS_BATTERY_EVENTS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BATTERY_STATUS_UNKNOWN = 0,
    BATTERY_STATUS_CHARGING,
    BATTERY_STATUS_DISCHARGING,
    BATTERY_STATUS_NOT_CHARGING,
    BATTERY_STATUS_FULL
} battery_status_t;

typedef enum {
    BATTERY_HEALTH_UNKNOWN = 0,
    BATTERY_HEALTH_GOOD,
    BATTERY_HEALTH_OVERHEAT,
    BATTERY_HEALTH_DEAD,
    BATTERY_HEALTH_OVERVOLTAGE,
    BATTERY_HEALTH_UNSPEC_FAILURE,
    BATTERY_HEALTH_COLD,
    BATTERY_HEALTH_WATCHDOG_TIMER_EXPIRE,
    BATTERY_HEALTH_SAFETY_TIMER_EXPIRE
} battery_health_t;

typedef struct battery_info {
    battery_status_t status;
    battery_health_t health;
    int32_t current_ua;
    int32_t voltage_uv;
    int32_t charge_full_uah;
    int32_t charge_now_uah;
    int32_t capacity_percent;
    int32_t temp_10ths_dc;
    bool present;
} battery_info_t;

int battery_get_info(int battery_id, battery_info_t *info);

#endif /* BHARAT_OS_BATTERY_EVENTS_H */
