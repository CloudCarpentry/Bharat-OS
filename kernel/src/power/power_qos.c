#include "power/power_qos.h"
#include <stddef.h>

/* Simple QoS aggregation framework */

static power_qos_request_t *pqos_lists[5] = {NULL, NULL, NULL, NULL, NULL};

int power_qos_add_request(power_qos_request_t *req) {
    if (!req) return -1;
    if (req->req_class > PQOS_CLASS_THERMAL_PREF) return -1;

    /* Add to the head of the list */
    req->next = pqos_lists[req->req_class];
    pqos_lists[req->req_class] = req;

    return 0;
}

int power_qos_update_request(power_qos_request_t *req, uint32_t new_value) {
    if (!req) return -1;
    if (req->req_class > PQOS_CLASS_THERMAL_PREF) return -1;

    /* Just update the value; aggregation happens on demand */
    req->value = new_value;

    return 0;
}

int power_qos_remove_request(power_qos_request_t *req) {
    if (!req) return -1;
    if (req->req_class > PQOS_CLASS_THERMAL_PREF) return -1;

    power_qos_request_t **curr = &pqos_lists[req->req_class];
    while (*curr) {
        if (*curr == req) {
            *curr = req->next;
            req->next = NULL;
            return 0;
        }
        curr = &((*curr)->next);
    }

    return -1; /* Not found */
}

int power_qos_get_effective(pqos_class_t req_class, uint32_t *effective_val) {
    if (!effective_val) return -1;
    if (req_class > PQOS_CLASS_THERMAL_PREF) return -1;

    power_qos_request_t *curr = pqos_lists[req_class];

    if (!curr) {
        /* Default values if no requests */
        switch (req_class) {
            case PQOS_CLASS_MIN_PERF: *effective_val = 0; break;
            case PQOS_CLASS_MAX_PERF: *effective_val = 0xFFFFFFFF; break;
            case PQOS_CLASS_CPU_LATENCY: *effective_val = 0xFFFFFFFF; break;
            case PQOS_CLASS_NO_DEEP_IDLE: *effective_val = 0; break;
            case PQOS_CLASS_THERMAL_PREF: *effective_val = 0; break;
            default: return -1;
        }
        return 0;
    }

    /* Aggregate based on class */
    uint32_t agg_val = curr->value;
    curr = curr->next;

    while (curr) {
        switch (req_class) {
            case PQOS_CLASS_MIN_PERF:
            case PQOS_CLASS_NO_DEEP_IDLE:
                if (curr->value > agg_val) agg_val = curr->value; /* MAX */
                break;
            case PQOS_CLASS_MAX_PERF:
            case PQOS_CLASS_CPU_LATENCY:
                if (curr->value < agg_val) agg_val = curr->value; /* MIN */
                break;
            case PQOS_CLASS_THERMAL_PREF:
                if (curr->value > agg_val) agg_val = curr->value; /* MAX */
                break;
        }
        curr = curr->next;
    }

    *effective_val = agg_val;
    return 0;
}
