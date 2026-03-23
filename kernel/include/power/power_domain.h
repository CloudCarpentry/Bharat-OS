#ifndef BHARAT_OS_POWER_DOMAIN_H
#define BHARAT_OS_POWER_DOMAIN_H

#include "power/power.h"

typedef enum {
    POWER_STATE_ON,
    POWER_STATE_CLOCK_GATED,
    POWER_STATE_RETENTION,
    POWER_STATE_SUSPEND,
    POWER_STATE_OFF
} power_state_t;

typedef struct power_domain_caps {
    uint32_t flags;
} power_domain_caps_t;

struct power_domain_ops {
    int (*get_state)(power_domain_t *pd, power_state_t *out);
    int (*set_state)(power_domain_t *pd, power_state_t target);
    int (*get_caps)(power_domain_t *pd, power_domain_caps_t *out);
};

struct power_domain {
    int id;
    const char *name;
    struct power_domain_ops *ops;
    power_state_t current_state;
    void *priv;
};

int power_domain_register(power_domain_t *pd);
int power_domain_get_state(power_domain_t *pd, power_state_t *out);
int power_domain_set_state(power_domain_t *pd, power_state_t target);
int power_domain_get_caps(power_domain_t *pd, power_domain_caps_t *out);

#endif /* BHARAT_OS_POWER_DOMAIN_H */
