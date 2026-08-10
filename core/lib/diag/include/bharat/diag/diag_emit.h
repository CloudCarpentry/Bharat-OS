/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_DIAG_EMIT_H
#define BHARAT_DIAG_EMIT_H
#include "bharat/diag/diag_ring.h"
typedef uint64_t (*bh_diag_timestamp_fn_t)(void *context);
typedef struct bh_diag_sink { bh_diag_ring_t *ring; bh_diag_timestamp_fn_t timestamp; void *timestamp_context; uint32_t source_id; uint16_t cpu_id; uint8_t source_kind; uint8_t reserved; } bh_diag_sink_t;
bh_status_t bh_diag_emit(bh_diag_sink_t *sink, uint16_t event_type, uint8_t severity, uint16_t subsystem_id, const void *payload, uint32_t payload_size);
#endif
