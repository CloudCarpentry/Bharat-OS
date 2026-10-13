#include "registry.h"
#include <string.h>
#include <bharat/uapi/service_status.h>
#include <bharat/bh_native.h>

void registry_init(metrics_registry_t *registry) {
    memset(registry, 0, sizeof(metrics_registry_t));
}

int32_t registry_register_metric(metrics_registry_t *registry, const bharat_counter_desc_t *desc, uint32_t *counter_id_out) {
    if (!registry || !desc || !counter_id_out) return BHARAT_STATUS_ERR_INVALID_ARG;

    // Check if already registered
    for (uint32_t i = 0; i < MAX_METRICS; i++) {
        if (registry->metrics[i].active && registry->metrics[i].desc.counter_id == desc->counter_id) {
            return BHARAT_STATUS_ERR_ALREADY_EXISTS;
        }
    }

    // Find free slot
    for (uint32_t i = 0; i < MAX_METRICS; i++) {
        if (!registry->metrics[i].active) {
            registry->metrics[i].desc = *desc;
            registry->metrics[i].snapshot.counter_id = desc->counter_id;
            registry->metrics[i].snapshot.value = 0;
            bh_time_get(BH_CLOCK_MONOTONIC, &registry->metrics[i].snapshot.timestamp_ns);
            registry->metrics[i].last_sequence = 0;
            registry->metrics[i].active = true;
            registry->active_count++;
            *counter_id_out = desc->counter_id;
            return BHARAT_STATUS_OK;
        }
    }

    registry->overflow_count++;
    return BHARAT_STATUS_ERR_NO_MEMORY;
}

int32_t registry_update_counter(metrics_registry_t *registry, uint32_t counter_id, uint64_t value, uint64_t sequence) {
    if (!registry) return BHARAT_STATUS_ERR_INVALID_ARG;

    for (uint32_t i = 0; i < MAX_METRICS; i++) {
        if (registry->metrics[i].active && registry->metrics[i].desc.counter_id == counter_id) {
            if (registry->metrics[i].desc.type != BHARAT_COUNTER_TYPE_MONOTONIC) {
                return BHARAT_STATUS_ERR_INVALID_ARG;
            }
            if (sequence <= registry->metrics[i].last_sequence && sequence != 0) {
                // Stale update
                return BHARAT_STATUS_ERR_BAD_STATE;
            }
            if (value < registry->metrics[i].snapshot.value) {
                // Counter overflow/invalid update for monotonic
                return BHARAT_STATUS_ERR_INVALID_ARG;
            }
            registry->metrics[i].snapshot.value = value;
            bh_time_get(BH_CLOCK_MONOTONIC, &registry->metrics[i].snapshot.timestamp_ns);
            if (sequence != 0) registry->metrics[i].last_sequence = sequence;
            return BHARAT_STATUS_OK;
        }
    }
    return BHARAT_STATUS_ERR_NOT_FOUND;
}

int32_t registry_update_gauge(metrics_registry_t *registry, uint32_t counter_id, uint64_t value, uint64_t sequence) {
    if (!registry) return BHARAT_STATUS_ERR_INVALID_ARG;

    for (uint32_t i = 0; i < MAX_METRICS; i++) {
        if (registry->metrics[i].active && registry->metrics[i].desc.counter_id == counter_id) {
            if (registry->metrics[i].desc.type != BHARAT_COUNTER_TYPE_GAUGE) {
                return BHARAT_STATUS_ERR_INVALID_ARG;
            }
            if (sequence <= registry->metrics[i].last_sequence && sequence != 0) {
                // Stale update
                return BHARAT_STATUS_ERR_BAD_STATE;
            }
            registry->metrics[i].snapshot.value = value;
            bh_time_get(BH_CLOCK_MONOTONIC, &registry->metrics[i].snapshot.timestamp_ns);
            if (sequence != 0) registry->metrics[i].last_sequence = sequence;
            return BHARAT_STATUS_OK;
        }
    }
    return BHARAT_STATUS_ERR_NOT_FOUND;
}

int32_t registry_query_metric(metrics_registry_t *registry, uint32_t counter_id, bharat_counter_snapshot_t *snapshot_out) {
    if (!registry || !snapshot_out) return BHARAT_STATUS_ERR_INVALID_ARG;

    for (uint32_t i = 0; i < MAX_METRICS; i++) {
        if (registry->metrics[i].active && registry->metrics[i].desc.counter_id == counter_id) {
            *snapshot_out = registry->metrics[i].snapshot;
            return BHARAT_STATUS_OK;
        }
    }
    return BHARAT_STATUS_ERR_NOT_FOUND;
}

int32_t registry_reset_metric(metrics_registry_t *registry, uint32_t counter_id) {
    if (!registry) return BHARAT_STATUS_ERR_INVALID_ARG;

    for (uint32_t i = 0; i < MAX_METRICS; i++) {
        if (registry->metrics[i].active && registry->metrics[i].desc.counter_id == counter_id) {
             if (registry->metrics[i].desc.type != BHARAT_COUNTER_TYPE_MONOTONIC) {
                 return BHARAT_STATUS_ERR_INVALID_ARG; // Only counters can be reset
             }
             registry->metrics[i].snapshot.value = 0;
             bh_time_get(BH_CLOCK_MONOTONIC, &registry->metrics[i].snapshot.timestamp_ns);
             return BHARAT_STATUS_OK;
        }
    }
    return BHARAT_STATUS_ERR_NOT_FOUND;
}
