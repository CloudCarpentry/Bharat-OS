/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_DIAG_COLLECTOR_H
#define BHARAT_DIAG_COLLECTOR_H
#include "bharat/diag/diag_ring.h"
#include "diag/health.h"
typedef struct bh_diag_collector_stats { uint64_t accepted, malformed, unsupported, sequence_gaps; } bh_diag_collector_stats_t;
typedef struct bh_diag_collector { bh_diag_collector_stats_t stats; uint64_t last_sequence; uint32_t source_id; uint8_t have_sequence; uint8_t degraded; uint16_t reserved; } bh_diag_collector_t;
typedef bh_status_t (*bh_diag_record_consumer_t)(void *context, const bh_diag_record_t *record);
void bh_diag_collector_init(bh_diag_collector_t *collector, uint32_t source_id);
bh_status_t bh_diag_collector_consume(bh_diag_collector_t *collector, bh_diag_ring_t *ring, bh_diag_record_consumer_t consumer, void *context);
void bh_diag_collector_health(const bh_diag_collector_t *collector, uint64_t now_ns, uint64_t uptime_ns, bh_diag_health_snapshot_t *snapshot);
void bh_diag_collector_get_stats(const bh_diag_collector_t *collector, bh_diag_collector_stats_t *stats);
#endif
