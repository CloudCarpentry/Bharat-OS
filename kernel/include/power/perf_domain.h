#ifndef BHARAT_OS_PERF_DOMAIN_H
#define BHARAT_OS_PERF_DOMAIN_H

#include <stdint.h>
#include <stddef.h>

typedef struct perf_domain perf_domain_t;

typedef struct perf_state {
    uint32_t id;
    uint32_t freq_khz;
    uint32_t voltage_mv;
} perf_state_t;

typedef struct perf_limit {
    uint32_t min_id;
    uint32_t max_id;
    uint32_t floor_id;
    uint32_t cap_id;
} perf_limit_t;

typedef struct perf_telemetry {
    uint64_t total_residency_ns;
    uint32_t current_state_id;
} perf_telemetry_t;

struct perf_domain_ops {
    int (*list_states)(perf_domain_t *pd, perf_state_t *buf, size_t *count);
    int (*set_state)(perf_domain_t *pd, uint32_t state_id);
    int (*set_limits)(perf_domain_t *pd, perf_limit_t *limits);
    int (*get_telemetry)(perf_domain_t *pd, perf_telemetry_t *out);
};

struct perf_domain {
    int id;
    const char *name;
    struct perf_domain_ops *ops;
    uint32_t current_state;
    void *priv;
};

int perf_domain_register(perf_domain_t *pd);
int perf_domain_set_state(perf_domain_t *pd, uint32_t state_id);
int perf_domain_set_limits(perf_domain_t *pd, perf_limit_t *limits);

#endif /* BHARAT_OS_PERF_DOMAIN_H */
