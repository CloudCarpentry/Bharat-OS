#ifndef BHARAT_OS_POWER_EVENTS_H
#define BHARAT_OS_POWER_EVENTS_H

#include <stdint.h>

typedef enum {
    PWR_EVENT_PRE_SUSPEND,
    PWR_EVENT_POST_RESUME,
    PWR_EVENT_THERMAL_TRIP,
    PWR_EVENT_BATTERY_LOW,
    PWR_EVENT_BATTERY_CRITICAL,
    PWR_EVENT_BROWNOUT_WARNING,
    PWR_EVENT_EMERGENCY_SHUTDOWN
} power_event_type_t;

typedef void (*power_event_cb_t)(power_event_type_t type, void *data, void *priv);

typedef struct power_notifier_block {
    power_event_cb_t callback;
    void *priv;
    struct power_notifier_block *next;
} power_notifier_block_t;

int power_register_notifier(power_notifier_block_t *nb);
int power_unregister_notifier(power_notifier_block_t *nb);
void power_broadcast_event(power_event_type_t type, void *data);

#endif /* BHARAT_OS_POWER_EVENTS_H */
