#ifndef TELEMETRYMGR_REGISTRY_H
#define TELEMETRYMGR_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/system/telemetry.h>

#define MAX_METRICS 256

typedef struct {
    bharat_counter_desc_t desc;
    bharat_counter_snapshot_t snapshot;
    uint64_t last_sequence;
    bool active;
} metric_entry_t;

typedef struct {
    metric_entry_t metrics[MAX_METRICS];
    uint32_t active_count;
    uint32_t overflow_count;
    uint32_t invalid_message_count;
} metrics_registry_t;

void registry_init(metrics_registry_t *registry);
int32_t registry_register_metric(metrics_registry_t *registry, const bharat_counter_desc_t *desc, uint32_t *counter_id_out);
int32_t registry_update_counter(metrics_registry_t *registry, uint32_t counter_id, uint64_t value, uint64_t sequence);
int32_t registry_update_gauge(metrics_registry_t *registry, uint32_t counter_id, uint64_t value, uint64_t sequence);
int32_t registry_query_metric(metrics_registry_t *registry, uint32_t counter_id, bharat_counter_snapshot_t *snapshot_out);
int32_t registry_reset_metric(metrics_registry_t *registry, uint32_t counter_id);

#endif // TELEMETRYMGR_REGISTRY_H
