#include "power/power_domain.h"
#include <stddef.h>

/* Simple fixed-size registry for phase 1 */
#define MAX_POWER_DOMAINS 32

static power_domain_t *power_domains[MAX_POWER_DOMAINS];
static int num_power_domains = 0;

int power_domain_register(power_domain_t *pd) {
    if (!pd || !pd->ops || !pd->ops->get_state || !pd->ops->set_state || !pd->ops->get_caps) {
        return -1;
    }

    if (num_power_domains >= MAX_POWER_DOMAINS) {
        return -1;
    }

    power_domains[num_power_domains++] = pd;
    return 0;
}

int power_domain_get_state(power_domain_t *pd, power_state_t *out) {
    if (!pd || !out) return -1;
    return pd->ops->get_state(pd, out);
}

int power_domain_set_state(power_domain_t *pd, power_state_t target) {
    if (!pd) return -1;

    int ret = pd->ops->set_state(pd, target);
    if (ret == 0) {
        pd->current_state = target;
    }
    return ret;
}

int power_domain_get_caps(power_domain_t *pd, power_domain_caps_t *out) {
    if (!pd || !out) return -1;
    return pd->ops->get_caps(pd, out);
}
