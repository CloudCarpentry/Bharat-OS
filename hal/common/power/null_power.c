#include "power/power_domain.h"
#include <stddef.h>

/* Null power implementation provides dummy generic ops */

static int null_pd_get_state(power_domain_t *pd, power_state_t *out) {
    if (!pd || !out) return -1;
    *out = POWER_STATE_ON;
    return 0;
}

static int null_pd_set_state(power_domain_t *pd, power_state_t target) {
    if (!pd) return -1;
    (void)target;
    return 0; /* Success but does nothing */
}

static int null_pd_get_caps(power_domain_t *pd, power_domain_caps_t *out) {
    if (!pd || !out) return -1;
    out->flags = 0; /* No capabilities */
    return 0;
}

struct power_domain_ops null_power_domain_ops = {
    .get_state = null_pd_get_state,
    .set_state = null_pd_set_state,
    .get_caps = null_pd_get_caps
};
