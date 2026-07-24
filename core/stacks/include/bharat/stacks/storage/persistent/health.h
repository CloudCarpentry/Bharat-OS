#ifndef BHARAT_STACKS_STORAGE_PERSISTENT_HEALTH_H
#define BHARAT_STACKS_STORAGE_PERSISTENT_HEALTH_H

#include "bharat/stacks/storage/block.h"

typedef struct {
    uint64_t requests_submitted;
    uint64_t requests_completed;
    uint64_t requests_failed;
    uint64_t requests_cancelled;
    uint64_t requests_timed_out;

    uint64_t reads_completed;
    uint64_t writes_completed;
    uint64_t flushes_completed;
    uint64_t discards_completed;

    uint64_t bytes_read;
    uint64_t bytes_written;

    uint64_t queue_full_events;
    uint64_t reset_count;
    uint64_t device_removal_count;
    uint64_t integrity_error_count;
    uint64_t encryption_error_count;
    uint64_t rollback_rejection_count;
} io_health_counters_t;

typedef struct {
    bool available;

    uint64_t media_errors;
    uint64_t unsafe_shutdowns;
    uint64_t corrected_errors;
    uint64_t bad_blocks;
    uint64_t wear_percent;
    uint64_t temperature_mc;
    uint64_t power_on_hours;
    uint64_t available_spare_percent;
} io_device_native_health_t;

typedef struct {
    io_device_id_t device_id;
    io_device_role_t role;
    io_device_state_t state;
    io_health_counters_t counters;
    io_device_native_health_t native_health;
} io_health_snapshot_t;

io_status_t block_device_health_snapshot(
    io_device_id_t device_id,
    io_health_snapshot_t *out);

// API to increment counters
void storage_health_inc_submitted(io_device_id_t device_id);
void storage_health_inc_completed(io_device_id_t device_id);
void storage_health_inc_failed(io_device_id_t device_id);
void storage_health_inc_cancelled(io_device_id_t device_id);
void storage_health_inc_timed_out(io_device_id_t device_id);
void storage_health_inc_op_completed(io_device_id_t device_id, io_opcode_t opcode, uint64_t block_count, uint32_t block_size);
void storage_health_inc_queue_full(io_device_id_t device_id);
void storage_health_inc_reset(io_device_id_t device_id);
void storage_health_inc_removal(io_device_id_t device_id);
void storage_health_inc_integrity_error(io_device_id_t device_id);
void storage_health_inc_encryption_error(io_device_id_t device_id);
void storage_health_inc_rollback_rejection(io_device_id_t device_id);

void storage_health_set_native_health(io_device_id_t device_id, const io_device_native_health_t *native);

#endif /* BHARAT_STACKS_STORAGE_PERSISTENT_HEALTH_H */
