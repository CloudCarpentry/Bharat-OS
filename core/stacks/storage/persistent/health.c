#include "bharat/stacks/storage/persistent/health.h"
#include <lib/base/string.h>

#define MAX_HEALTH_DEVICES 32U

typedef struct {
    io_device_id_t device_id;
    io_health_counters_t counters;
    io_device_native_health_t native;
    bool active;
} device_health_entry_t;

static device_health_entry_t g_device_health[MAX_HEALTH_DEVICES];

static device_health_entry_t* get_or_create_entry(io_device_id_t device_id) {
    for (uint32_t i = 0; i < MAX_HEALTH_DEVICES; ++i) {
        if (g_device_health[i].active && g_device_health[i].device_id == device_id) {
            return &g_device_health[i];
        }
    }
    for (uint32_t i = 0; i < MAX_HEALTH_DEVICES; ++i) {
        if (!g_device_health[i].active) {
            g_device_health[i].device_id = device_id;
            g_device_health[i].active = true;
            memset(&g_device_health[i].counters, 0, sizeof(io_health_counters_t));
            memset(&g_device_health[i].native, 0, sizeof(io_device_native_health_t));
            return &g_device_health[i];
        }
    }
    return NULL;
}

io_status_t block_device_health_snapshot(io_device_id_t device_id, io_health_snapshot_t *out) {
    if (!out) return IO_STATUS_INVALID_ARGUMENT;

    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;

    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (!entry) return IO_STATUS_NOT_READY;

    out->device_id = device_id;
    out->role = dev->role;
    out->state = dev->state;
    out->counters = entry->counters;
    out->native_health = entry->native;

    return IO_STATUS_OK;
}

void storage_health_inc_submitted(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.requests_submitted++;
    }
}

void storage_health_inc_completed(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.requests_completed++;
    }
}

void storage_health_inc_failed(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.requests_failed++;
    }
}

void storage_health_inc_cancelled(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.requests_cancelled++;
    }
}

void storage_health_inc_timed_out(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.requests_timed_out++;
    }
}

void storage_health_inc_op_completed(io_device_id_t device_id, io_opcode_t opcode, uint64_t block_count, uint32_t block_size) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        switch (opcode) {
            case IO_OP_READ:
                entry->counters.reads_completed++;
                entry->counters.bytes_read += block_count * (uint64_t)block_size;
                break;
            case IO_OP_WRITE:
                entry->counters.writes_completed++;
                entry->counters.bytes_written += block_count * (uint64_t)block_size;
                break;
            case IO_OP_FLUSH:
                entry->counters.flushes_completed++;
                break;
            case IO_OP_DISCARD:
                entry->counters.discards_completed++;
                break;
            default:
                break;
        }
    }
}

void storage_health_inc_queue_full(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.queue_full_events++;
    }
}

void storage_health_inc_reset(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.reset_count++;
    }
}

void storage_health_inc_removal(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.device_removal_count++;
    }
}

void storage_health_inc_integrity_error(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.integrity_error_count++;
    }
}

void storage_health_inc_encryption_error(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.encryption_error_count++;
    }
}

void storage_health_inc_rollback_rejection(io_device_id_t device_id) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry) {
        entry->counters.rollback_rejection_count++;
    }
}

void storage_health_set_native_health(io_device_id_t device_id, const io_device_native_health_t *native) {
    device_health_entry_t *entry = get_or_create_entry(device_id);
    if (entry && native) {
        entry->native = *native;
    }
}
