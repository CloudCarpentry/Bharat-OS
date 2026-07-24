#ifndef BHARAT_STACKS_STORAGE_PERSISTENT_EVENT_LOG_H
#define BHARAT_STACKS_STORAGE_PERSISTENT_EVENT_LOG_H

#include "bharat/stacks/storage/block.h"

#define EVENT_LOG_MAGIC 0x45564E54U      // "EVNT"
#define EVENT_SUPER_MAGIC 0x53555052U    // "SUPR"
#define EVENT_COMMIT_MAGIC 0xC001CAFEL

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t record_type;

    uint64_t sequence;
    uint64_t timestamp_ns;

    uint32_t payload_length;
    uint32_t payload_crc;

    uint32_t header_crc;
    uint32_t flags;
} persistent_event_header_t;

typedef struct {
    uint32_t commit_magic;
    uint32_t total_crc;
} persistent_event_footer_t;

typedef struct {
    uint32_t magic;
    uint32_t format_version;
    uint32_t max_sectors;
    uint32_t reserved;
    uint32_t crc;
} persistent_event_super_t;

// API functions
io_status_t event_log_init(io_device_id_t device_id);
io_status_t event_log_append(io_device_id_t device_id, uint16_t record_type, const void *payload, uint32_t payload_len);
io_status_t event_log_recover(io_device_id_t device_id, uint64_t *out_last_sequence);
io_status_t event_log_read_record(io_device_id_t device_id, uint64_t sequence, void *payload_out, uint32_t max_len, uint32_t *out_len);

#endif /* BHARAT_STACKS_STORAGE_PERSISTENT_EVENT_LOG_H */
