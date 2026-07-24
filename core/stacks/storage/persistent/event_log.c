#include "bharat/stacks/storage/persistent/event_log.h"
#include "bharat/stacks/storage/persistent/health.h"
#include <lib/base/string.h>

static uint64_t g_next_sequence = 1ULL;
static uint32_t g_next_sector = 1U;
static bool g_recovered = false;

static uint32_t crc32_incremental(uint32_t crc, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static uint32_t crc32(const uint8_t *data, size_t len) {
    return ~crc32_incremental(0xFFFFFFFFU, data, len);
}

static io_status_t sync_block_io(io_device_id_t device_id, io_opcode_t opcode, uint64_t lba, void *buf) {
    io_channel_config_t config = {
        .queue_id = 0,
        .queue_depth = 16,
        .enable_polling = true
    };
    void *channel = NULL;
    io_status_t status = block_channel_open(device_id, &config, &channel);
    if (status != IO_STATUS_OK) return status;

    io_sg_entry_t sg = {
        .phys_addr = (uintptr_t)buf,
        .length = 512
    };

    io_request_t req = {
        .id = 5000ULL + lba + opcode * 100000ULL,
        .device_id = device_id,
        .opcode = opcode,
        .lba = lba,
        .block_count = (opcode == IO_OP_FLUSH) ? 0 : 1,
        .segments = (opcode == IO_OP_FLUSH) ? NULL : &sg,
        .segment_count = (opcode == IO_OP_FLUSH) ? 0 : 1,
    };

    status = block_submit(device_id, channel, &req);
    if (status != IO_STATUS_OK) {
        block_channel_close(device_id, channel);
        return status;
    }

    extern void memblk_tick(void) __attribute__((weak));
    bool completed = false;
    io_completion_t comp;

    for (uint32_t retries = 0; retries < 10000U; ++retries) {
        if (memblk_tick) {
            memblk_tick();
        }

        uint32_t count = 0;
        if (block_poll_completions(device_id, channel, &comp, 1, &count) == IO_STATUS_OK && count == 1) {
            if (comp.id == req.id) {
                status = comp.status;
                completed = true;
                break;
            }
        }
    }

    block_channel_close(device_id, channel);
    return completed ? status : IO_STATUS_TIMEOUT;
}

io_status_t event_log_init(io_device_id_t device_id) {
    g_next_sequence = 1ULL;
    g_next_sector = 1U;
    g_recovered = true;

    // Write superblock
    persistent_event_super_t sb = {
        .magic = EVENT_SUPER_MAGIC,
        .format_version = 1,
        .max_sectors = 1000,
        .reserved = 0
    };
    sb.crc = crc32((const uint8_t*)&sb, offsetof(persistent_event_super_t, crc));

    uint8_t buf[512];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &sb, sizeof(sb));

    io_status_t status = sync_block_io(device_id, IO_OP_WRITE, 0, buf);
    if (status != IO_STATUS_OK) return status;

    // Zero out the rest of the log sectors to clear any legacy records
    memset(buf, 0, sizeof(buf));
    for (uint32_t i = 1; i <= 1000; ++i) {
        status = sync_block_io(device_id, IO_OP_WRITE, i, buf);
        if (status != IO_STATUS_OK) return status;
    }

    // Flush to ensure persistent
    status = sync_block_io(device_id, IO_OP_FLUSH, 0, NULL);
    return status;
}

io_status_t event_log_recover(io_device_id_t device_id, uint64_t *out_last_sequence) {
    if (!out_last_sequence) return IO_STATUS_INVALID_ARGUMENT;

    // Read superblock
    uint8_t buf[512];
    io_status_t status = sync_block_io(device_id, IO_OP_READ, 0, buf);
    if (status != IO_STATUS_OK) {
        storage_health_inc_integrity_error(device_id);
        return status;
    }

    persistent_event_super_t sb;
    memcpy(&sb, buf, sizeof(sb));

    uint32_t computed_sb_crc = crc32((const uint8_t*)&sb, offsetof(persistent_event_super_t, crc));
    if (sb.magic != EVENT_SUPER_MAGIC || sb.crc != computed_sb_crc) {
        storage_health_inc_integrity_error(device_id);
        return IO_STATUS_IO_ERROR; // Corrupt superblock
    }

    uint64_t max_seq = 0ULL;
    uint32_t max_seq_sector = 0U;
    bool found_any = false;

    for (uint32_t i = 1; i <= sb.max_sectors; ++i) {
        status = sync_block_io(device_id, IO_OP_READ, i, buf);
        if (status != IO_STATUS_OK) {
            continue; // Skip failed read or continue scanning
        }

        persistent_event_header_t header;
        memcpy(&header, buf, sizeof(header));

        if (header.magic == EVENT_LOG_MAGIC) {
            uint32_t computed_header_crc = crc32((const uint8_t*)&header, offsetof(persistent_event_header_t, header_crc));
            if (header.header_crc == computed_header_crc && header.payload_length <= 450) {
                // Verify commit footer
                persistent_event_footer_t footer;
                memcpy(&footer, buf + sizeof(persistent_event_header_t) + header.payload_length, sizeof(footer));

                if (footer.commit_magic == EVENT_COMMIT_MAGIC) {
                    uint32_t tcrc = crc32_incremental(0xFFFFFFFFU, (const uint8_t*)&header, sizeof(header));
                    tcrc = crc32_incremental(tcrc, buf + sizeof(persistent_event_header_t), header.payload_length);
                    tcrc = ~tcrc;

                    if (footer.total_crc == tcrc) {
                        found_any = true;
                        if (header.sequence > max_seq) {
                            max_seq = header.sequence;
                            max_seq_sector = i;
                        }
                    } else {
                        storage_health_inc_integrity_error(device_id);
                    }
                } else {
                    // Torn record or incomplete write detected
                    storage_health_inc_integrity_error(device_id);
                }
            } else {
                storage_health_inc_integrity_error(device_id);
            }
        } else {
            // Non-zero garbage indicates corruption/integrity error
            bool is_zero = true;
            for (size_t b = 0; b < 512; ++b) {
                if (buf[b] != 0) {
                    is_zero = false;
                    break;
                }
            }
            if (!is_zero) {
                storage_health_inc_integrity_error(device_id);
            }
        }
    }

    if (found_any) {
        *out_last_sequence = max_seq;
        g_next_sequence = max_seq + 1;
        g_next_sector = max_seq_sector + 1;
        if (g_next_sector > sb.max_sectors) {
            g_next_sector = 1;
        }
    } else {
        *out_last_sequence = 0;
        g_next_sequence = 1;
        g_next_sector = 1;
    }

    g_recovered = true;
    return IO_STATUS_OK;
}

io_status_t event_log_append(io_device_id_t device_id, uint16_t record_type, const void *payload, uint32_t payload_len) {
    if (payload_len > 450) return IO_STATUS_INVALID_ARGUMENT;

    if (!g_recovered) {
        uint64_t last_seq;
        io_status_t status = event_log_recover(device_id, &last_seq);
        if (status != IO_STATUS_OK) return status;
    }

    persistent_event_header_t header = {
        .magic = EVENT_LOG_MAGIC,
        .format_version = 1,
        .record_type = record_type,
        .sequence = g_next_sequence,
        .timestamp_ns = 12345678ULL,
        .payload_length = payload_len,
        .payload_crc = crc32(payload, payload_len),
        .flags = 0
    };
    header.header_crc = crc32((const uint8_t*)&header, offsetof(persistent_event_header_t, header_crc));

    uint8_t buf[512];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &header, sizeof(header));
    memcpy(buf + sizeof(header), payload, payload_len);

    persistent_event_footer_t footer = {
        .commit_magic = EVENT_COMMIT_MAGIC
    };
    uint32_t tcrc = crc32_incremental(0xFFFFFFFFU, (const uint8_t*)&header, sizeof(header));
    tcrc = crc32_incremental(tcrc, payload, payload_len);
    footer.total_crc = ~tcrc;

    memcpy(buf + sizeof(header) + payload_len, &footer, sizeof(footer));

    io_status_t status = sync_block_io(device_id, IO_OP_WRITE, g_next_sector, buf);
    if (status != IO_STATUS_OK) return status;

    // Enforce flush
    status = sync_block_io(device_id, IO_OP_FLUSH, 0, NULL);
    if (status != IO_STATUS_OK) return status;

    g_next_sequence++;
    g_next_sector++;
    if (g_next_sector > 1000) {
        g_next_sector = 1; // Wrap around to sector 1
    }

    return IO_STATUS_OK;
}

io_status_t event_log_read_record(io_device_id_t device_id, uint64_t sequence, void *payload_out, uint32_t max_len, uint32_t *out_len) {
    if (!payload_out || !out_len) return IO_STATUS_INVALID_ARGUMENT;

    uint8_t buf[512];
    for (uint32_t i = 1; i <= 1000; ++i) {
        io_status_t status = sync_block_io(device_id, IO_OP_READ, i, buf);
        if (status != IO_STATUS_OK) continue;

        persistent_event_header_t header;
        memcpy(&header, buf, sizeof(header));

        if (header.magic == EVENT_LOG_MAGIC && header.sequence == sequence) {
            uint32_t computed_header_crc = crc32((const uint8_t*)&header, offsetof(persistent_event_header_t, header_crc));
            if (header.header_crc == computed_header_crc && header.payload_length <= 450) {
                persistent_event_footer_t footer;
                memcpy(&footer, buf + sizeof(header) + header.payload_length, sizeof(footer));

                if (footer.commit_magic == EVENT_COMMIT_MAGIC) {
                    uint32_t tcrc = crc32_incremental(0xFFFFFFFFU, (const uint8_t*)&header, sizeof(header));
                    tcrc = crc32_incremental(tcrc, buf + sizeof(header), header.payload_length);
                    tcrc = ~tcrc;

                    if (footer.total_crc == tcrc) {
                        uint32_t copy_len = (header.payload_length < max_len) ? header.payload_length : max_len;
                        memcpy(payload_out, buf + sizeof(header), copy_len);
                        *out_len = header.payload_length;
                        return IO_STATUS_OK;
                    }
                }
            }
        }
    }

    return IO_STATUS_INVALID_ARGUMENT; // Not found or invalid
}
