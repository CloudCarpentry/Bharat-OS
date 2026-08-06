/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_DIAG_RING_H
#define BHARAT_DIAG_RING_H
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include "bharat/uapi/syscall/bh_syscall_status.h"
#include "diag/event.h"

#ifndef BHARAT_DIAG_MAX_PAYLOAD
#define BHARAT_DIAG_MAX_PAYLOAD 256u
#endif

typedef struct bh_diag_record { bh_diag_event_header_t header; uint8_t payload[BHARAT_DIAG_MAX_PAYLOAD]; } bh_diag_record_t;
typedef struct bh_diag_ring_slot { _Atomic uint64_t committed_sequence; bh_diag_record_t record; } bh_diag_ring_slot_t;
typedef struct bh_diag_ring_stats { uint64_t accepted, consumed, dropped, corrupt, high_watermark; } bh_diag_ring_stats_t;

/* Single writer and single reader own their respective indices.  The writer
 * publishes a completed slot with release; the reader observes it with acquire.
 * Slots and ring storage are caller-owned and remain live until reset/quiescence. */
typedef struct bh_diag_ring {
    bh_diag_ring_slot_t *slots;
    uint32_t capacity;
    uint32_t max_payload;
    _Atomic uint64_t write_position;
    _Atomic uint64_t read_position;
    _Atomic uint64_t accepted, consumed, dropped, corrupt, high_watermark;
} bh_diag_ring_t;

bh_status_t bh_diag_ring_init(bh_diag_ring_t *ring, bh_diag_ring_slot_t *slots, uint32_t capacity, uint32_t max_payload);
bh_status_t bh_diag_ring_try_write(bh_diag_ring_t *ring, const bh_diag_event_header_t *header, const void *payload);
bh_status_t bh_diag_ring_try_read(bh_diag_ring_t *ring, bh_diag_record_t *record);
bh_status_t bh_diag_ring_peek(bh_diag_ring_t *ring, bh_diag_record_t *record);
void bh_diag_ring_reset(bh_diag_ring_t *ring);
void bh_diag_ring_get_stats(const bh_diag_ring_t *ring, bh_diag_ring_stats_t *stats);
#endif
