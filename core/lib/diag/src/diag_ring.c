/* SPDX-License-Identifier: MIT */
#include "bharat/diag/diag_ring.h"
static void bytes_zero(void *destination, size_t size) {
    uint8_t *out = destination;
    for (size_t i = 0; i < size; ++i) out[i] = 0;
}
static void bytes_copy(void *destination, const void *source, size_t size) {
    uint8_t *out = destination;
    const uint8_t *in = source;
    for (size_t i = 0; i < size; ++i) out[i] = in[i];
}

static int ring_valid(const bh_diag_ring_t *ring) { return ring && ring->slots && ring->capacity && ring->max_payload <= BHARAT_DIAG_MAX_PAYLOAD; }
static void update_high_watermark(bh_diag_ring_t *ring, uint64_t value) {
    uint64_t old = atomic_load_explicit(&ring->high_watermark, memory_order_relaxed);
    while (old < value && !atomic_compare_exchange_weak_explicit(&ring->high_watermark, &old, value, memory_order_relaxed, memory_order_relaxed)) {}
}
bh_status_t bh_diag_ring_init(bh_diag_ring_t *ring, bh_diag_ring_slot_t *slots, uint32_t capacity, uint32_t max_payload) {
    if (!ring || !slots || !capacity || !max_payload || max_payload > BHARAT_DIAG_MAX_PAYLOAD) return BH_ERR_INVALID_ARGUMENT;
    bytes_zero(ring, sizeof(*ring)); ring->slots = slots; ring->capacity = capacity; ring->max_payload = max_payload;
    for (uint32_t i = 0; i < capacity; ++i) atomic_store_explicit(&slots[i].committed_sequence, 0, memory_order_relaxed);
    return BH_OK;
}
bh_status_t bh_diag_ring_try_write(bh_diag_ring_t *ring, const bh_diag_event_header_t *header, const void *payload) {
    if (!ring_valid(ring) || !header || (header->payload_size && !payload)) return BH_ERR_INVALID_ARGUMENT;
    if (header->abi_version != BH_DIAG_ABI_VERSION || header->header_size != sizeof(*header) || header->payload_size > ring->max_payload || header->severity >= BH_DIAG_SEVERITY_COUNT || header->source_kind >= BH_DIAG_SOURCE_KIND_COUNT) return BH_ERR_INVALID_ARGUMENT;
    uint64_t write = atomic_load_explicit(&ring->write_position, memory_order_relaxed);
    uint64_t read = atomic_load_explicit(&ring->read_position, memory_order_acquire);
    if (write - read >= ring->capacity) { atomic_fetch_add_explicit(&ring->dropped, 1, memory_order_relaxed); return BH_ERR_BUFFER_FULL; }
    bh_diag_ring_slot_t *slot = &ring->slots[write % ring->capacity];
    slot->record.header = *header; slot->record.header.sequence = write + 1;
    if (header->payload_size) bytes_copy(slot->record.payload, payload, header->payload_size);
    atomic_store_explicit(&slot->committed_sequence, write + 1, memory_order_release);
    atomic_store_explicit(&ring->write_position, write + 1, memory_order_release);
    atomic_fetch_add_explicit(&ring->accepted, 1, memory_order_relaxed); update_high_watermark(ring, write + 1 - read);
    return BH_OK;
}
static bh_status_t copy_next(bh_diag_ring_t *ring, bh_diag_record_t *record, int consume) {
    if (!ring_valid(ring) || !record) return BH_ERR_INVALID_ARGUMENT;
    uint64_t read = atomic_load_explicit(&ring->read_position, memory_order_relaxed);
    uint64_t write = atomic_load_explicit(&ring->write_position, memory_order_acquire);
    if (read == write) return BH_ERR_NOT_FOUND;
    bh_diag_ring_slot_t *slot = &ring->slots[read % ring->capacity];
    uint64_t expected = read + 1;
    if (atomic_load_explicit(&slot->committed_sequence, memory_order_acquire) != expected || slot->record.header.sequence != expected || slot->record.header.header_size != sizeof(bh_diag_event_header_t) || slot->record.header.payload_size > ring->max_payload) { atomic_fetch_add_explicit(&ring->corrupt, 1, memory_order_relaxed); return BH_ERR_FAULT; }
    *record = slot->record;
    if (consume) { atomic_store_explicit(&slot->committed_sequence, 0, memory_order_release); atomic_store_explicit(&ring->read_position, read + 1, memory_order_release); atomic_fetch_add_explicit(&ring->consumed, 1, memory_order_relaxed); }
    return BH_OK;
}
bh_status_t bh_diag_ring_try_read(bh_diag_ring_t *ring, bh_diag_record_t *record) { return copy_next(ring, record, 1); }
bh_status_t bh_diag_ring_peek(bh_diag_ring_t *ring, bh_diag_record_t *record) { return copy_next(ring, record, 0); }
void bh_diag_ring_reset(bh_diag_ring_t *ring) { if (!ring_valid(ring)) return; for (uint32_t i=0;i<ring->capacity;++i) atomic_store_explicit(&ring->slots[i].committed_sequence,0,memory_order_relaxed); atomic_store(&ring->write_position,0); atomic_store(&ring->read_position,0); atomic_store(&ring->accepted,0); atomic_store(&ring->consumed,0); atomic_store(&ring->dropped,0); atomic_store(&ring->corrupt,0); atomic_store(&ring->high_watermark,0); }
void bh_diag_ring_get_stats(const bh_diag_ring_t *ring, bh_diag_ring_stats_t *stats) { if (!ring_valid(ring) || !stats) return; stats->accepted=atomic_load(&ring->accepted); stats->consumed=atomic_load(&ring->consumed); stats->dropped=atomic_load(&ring->dropped); stats->corrupt=atomic_load(&ring->corrupt); stats->high_watermark=atomic_load(&ring->high_watermark); }
