#ifndef BHARAT_OS_POWER_QOS_H
#define BHARAT_OS_POWER_QOS_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    PQOS_CLASS_MIN_PERF,
    PQOS_CLASS_MAX_PERF,
    PQOS_CLASS_CPU_LATENCY,
    PQOS_CLASS_NO_DEEP_IDLE,
    PQOS_CLASS_THERMAL_PREF
} pqos_class_t;

typedef struct power_qos_request power_qos_request_t;

struct power_qos_request {
    pqos_class_t req_class;
    uint32_t value;
    struct power_qos_request *next;
    void *priv;
};

int power_qos_add_request(power_qos_request_t *req);
int power_qos_update_request(power_qos_request_t *req, uint32_t new_value);
int power_qos_remove_request(power_qos_request_t *req);
int power_qos_get_effective(pqos_class_t req_class, uint32_t *effective_val);

#endif /* BHARAT_OS_POWER_QOS_H */
