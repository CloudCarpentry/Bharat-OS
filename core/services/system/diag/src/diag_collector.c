/* SPDX-License-Identifier: MIT */
#include "bharat/diag/diag_collector.h"
static void collector_zero(void *destination, size_t size) { uint8_t *out=destination; for(size_t i=0;i<size;++i)out[i]=0; }
void bh_diag_collector_init(bh_diag_collector_t *c, uint32_t source_id) { if (c) { collector_zero(c,sizeof(*c)); c->source_id=source_id; } }
bh_status_t bh_diag_collector_consume(bh_diag_collector_t *c, bh_diag_ring_t *ring, bh_diag_record_consumer_t consumer, void *context) {
    if (!c || !ring || !consumer) return BH_ERR_INVALID_ARGUMENT;
    bh_diag_record_t record; bh_status_t st=bh_diag_ring_try_read(ring,&record); if (st != BH_OK) { if (st==BH_ERR_FAULT) { c->stats.malformed++; c->degraded=1; } return st; }
    if (record.header.abi_version != BH_DIAG_ABI_VERSION) { c->stats.unsupported++; c->degraded=1; return BH_ERR_NOT_SUPPORTED; }
    if (record.header.header_size != sizeof(record.header) || record.header.payload_size > BHARAT_DIAG_MAX_PAYLOAD) { c->stats.malformed++; c->degraded=1; return BH_ERR_INVALID_ARGUMENT; }
    if (c->have_sequence && record.header.sequence > c->last_sequence + 1) c->stats.sequence_gaps += record.header.sequence-c->last_sequence-1;
    if (c->have_sequence && record.header.sequence <= c->last_sequence) { c->stats.malformed++; c->degraded=1; return BH_ERR_BAD_STATE; }
    st=consumer(context,&record); if (st != BH_OK) return st;
    c->last_sequence=record.header.sequence; c->have_sequence=1; c->stats.accepted++; return BH_OK;
}
void bh_diag_collector_health(const bh_diag_collector_t *c,uint64_t now,uint64_t uptime,bh_diag_health_snapshot_t *s) { if (!c||!s) return; collector_zero(s,sizeof(*s)); s->schema_version=BH_DIAG_HEALTH_SCHEMA_VERSION; s->state=c->degraded?BH_DIAG_HEALTH_DEGRADED:BH_DIAG_HEALTH_HEALTHY; s->source_id=c->source_id; s->timestamp_ns=now; s->uptime_ns=uptime; s->dropped_events=c->stats.sequence_gaps; }
void bh_diag_collector_get_stats(const bh_diag_collector_t *c,bh_diag_collector_stats_t *s) { if(c&&s)*s=c->stats; }
