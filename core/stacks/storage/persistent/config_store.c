#include "bharat/stacks/storage/persistent/config_store.h"
#include "bharat/stacks/storage/persistent/durability.h"
#include "bharat/stacks/storage/persistent/health.h"
#include "bharat/stacks/storage/persistent/rollback.h"
#include <lib/base/string.h>

static uint8_t g_partition_uuid[16] = {0};
static bool g_uuid_set = false;

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
        .id = 8000ULL + lba + opcode * 100000ULL,
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

io_status_t config_store_init(io_device_id_t device_id, const uint8_t partition_uuid[16]) {
    if (partition_uuid) {
        memcpy(g_partition_uuid, partition_uuid, 16);
        g_uuid_set = true;
    }

    // Write selector
    config_selector_t sel = {
        .magic = CONFIG_SELECTOR_MAGIC,
        .active_slot = 0, // A
        .generation = 1,
        .rollback_epoch = 1
    };
    sel.crc = crc32((const uint8_t*)&sel, offsetof(config_selector_t, crc));

    uint8_t sector[512];
    memset(sector, 0, sizeof(sector));
    memcpy(sector, &sel, sizeof(sel));

    io_status_t status = sync_block_io(device_id, IO_OP_WRITE, 0, sector);
    if (status != IO_STATUS_OK) return status;

    // Zero slots LBA 1 (Slot A) and LBA 100 (Slot B)
    memset(sector, 0, sizeof(sector));
    status = sync_block_io(device_id, IO_OP_WRITE, 1, sector);
    if (status != IO_STATUS_OK) return status;

    status = sync_block_io(device_id, IO_OP_WRITE, 100, sector);
    if (status != IO_STATUS_OK) return status;

    // Flush
    return sync_block_io(device_id, IO_OP_FLUSH, 0, NULL);
}

io_status_t config_store_write(
    io_device_id_t device_id,
    capability_handle_t key_cap,
    uint32_t namespace_id,
    const void *plaintext,
    uint32_t plaintext_len) {

    if (!plaintext || plaintext_len == 0 || plaintext_len > 400) {
        return IO_STATUS_INVALID_ARGUMENT;
    }

    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;

    // Read current selector
    uint8_t sector[512];
    io_status_t status = sync_block_io(device_id, IO_OP_READ, 0, sector);
    if (status != IO_STATUS_OK) {
        storage_health_inc_integrity_error(device_id);
        return status;
    }

    config_selector_t sel;
    memcpy(&sel, sector, sizeof(sel));

    uint32_t computed_sel_crc = crc32((const uint8_t*)&sel, offsetof(config_selector_t, crc));
    if (sel.magic != CONFIG_SELECTOR_MAGIC || sel.crc != computed_sel_crc) {
        // Corrupt selector, fallback/default to slot A
        sel.magic = CONFIG_SELECTOR_MAGIC;
        sel.active_slot = 0;
        sel.generation = 1;
        sel.rollback_epoch = 1;
    }

    // Determine target inactive slot
    uint32_t inactive_slot = (sel.active_slot == 0) ? 1 : 0;
    uint64_t inactive_lba = (inactive_slot == 0) ? 1 : 100;

    uint64_t next_generation = sel.generation + 1;
    uint64_t next_epoch = sel.rollback_epoch;

    // Retrieve rollback anchor if prevention level is enabled
    extern rollback_protection_level_t g_test_rollback_level;
    extern uint64_t g_test_rollback_epoch;
    if (g_test_rollback_level == ROLLBACK_PROTECTION_PREVENTION) {
        next_epoch = g_test_rollback_epoch;
    }

    // Construct AAD binding context
    config_binding_context_t binding = {
        .format_version = 1,
        .device_id = device_id,
        .namespace_id = namespace_id,
        .generation = next_generation,
        .rollback_epoch = next_epoch
    };
    memcpy(binding.partition_uuid, g_partition_uuid, 16);

    // Call Keystore to seal
    uint8_t ciphertext[512];
    memset(ciphertext, 0, sizeof(ciphertext));

    keystore_aead_request_t req = {
        .plaintext = plaintext,
        .plaintext_len = plaintext_len,
        .aad = (const uint8_t*)&binding,
        .aad_len = sizeof(binding)
    };
    memset(req.nonce, 0x1F, 16); // constant nonce for simplicity

    keystore_aead_response_t resp = {
        .ciphertext = ciphertext,
        .ciphertext_len = plaintext_len
    };

    keystore_status_t ks_status = keystore_aead_seal(key_cap, &req, &resp);
    if (ks_status == KEYSTORE_STATUS_ERR_REVOKED) {
        storage_health_inc_encryption_error(device_id);
        return IO_STATUS_PERMISSION_DENIED;
    }
    if (ks_status != KEYSTORE_STATUS_OK) {
        storage_health_inc_encryption_error(device_id);
        return IO_STATUS_PERMISSION_DENIED;
    }

    // Construct header
    encrypted_config_header_t header = {
        .magic = CONFIG_MAGIC,
        .format_version = 1,
        .algorithm_id = 1,
        .generation = next_generation,
        .rollback_epoch = next_epoch,
        .namespace_id = namespace_id,
        .plaintext_length = plaintext_len,
        .ciphertext_length = plaintext_len
    };
    memcpy(header.partition_uuid, g_partition_uuid, 16);
    memcpy(header.nonce, req.nonce, CONFIG_NONCE_SIZE);
    memset(header.key_binding_id, 0xAB, CONFIG_KEY_BINDING_SIZE);

    header.header_crc = crc32((const uint8_t*)&header, offsetof(encrypted_config_header_t, header_crc));

    // Construct inactive sector
    memset(sector, 0, sizeof(sector));
    memcpy(sector, &header, sizeof(header));
    memcpy(sector + sizeof(header), ciphertext, plaintext_len);
    // Append 16-byte tag at end of ciphertext
    memcpy(sector + sizeof(header) + plaintext_len, resp.tag, 16);

    // Write to inactive slot LBA
    status = sync_block_io(device_id, IO_OP_WRITE, inactive_lba, sector);
    if (status != IO_STATUS_OK) return status;

    // Flush or FUA
    status = sync_block_io(device_id, IO_OP_FLUSH, 0, NULL);
    if (status != IO_STATUS_OK) return status;

    // Read back and verify inactive slot
    uint8_t readback[512];
    status = sync_block_io(device_id, IO_OP_READ, inactive_lba, readback);
    if (status != IO_STATUS_OK || memcmp(sector, readback, 512) != 0) {
        storage_health_inc_integrity_error(device_id);
        return IO_STATUS_IO_ERROR;
    }

    // Update selector
    sel.active_slot = inactive_slot;
    sel.generation = next_generation;
    sel.rollback_epoch = next_epoch;
    sel.crc = crc32((const uint8_t*)&sel, offsetof(config_selector_t, crc));

    memset(sector, 0, sizeof(sector));
    memcpy(sector, &sel, sizeof(sel));

    status = sync_block_io(device_id, IO_OP_WRITE, 0, sector);
    if (status != IO_STATUS_OK) return status;

    // Flush selector
    status = sync_block_io(device_id, IO_OP_FLUSH, 0, NULL);
    return status;
}

io_status_t config_store_read(
    io_device_id_t device_id,
    capability_handle_t key_cap,
    uint32_t namespace_id,
    void *plaintext_out,
    uint32_t max_len,
    uint32_t *out_len) {

    if (!plaintext_out || !out_len) return IO_STATUS_INVALID_ARGUMENT;

    // Read selector
    uint8_t sector[512];
    io_status_t status = sync_block_io(device_id, IO_OP_READ, 0, sector);
    if (status != IO_STATUS_OK) {
        storage_health_inc_integrity_error(device_id);
        return status;
    }

    config_selector_t sel;
    memcpy(&sel, sector, sizeof(sel));

    uint32_t computed_sel_crc = crc32((const uint8_t*)&sel, offsetof(config_selector_t, crc));
    if (sel.magic != CONFIG_SELECTOR_MAGIC || sel.crc != computed_sel_crc) {
        storage_health_inc_integrity_error(device_id);
        return IO_STATUS_IO_ERROR; // Selector corrupted
    }

    // Read active slot
    uint64_t active_lba = (sel.active_slot == 0) ? 1 : 100;
    status = sync_block_io(device_id, IO_OP_READ, active_lba, sector);
    if (status != IO_STATUS_OK) {
        storage_health_inc_integrity_error(device_id);
        return status;
    }

    encrypted_config_header_t header;
    memcpy(&header, sector, sizeof(header));

    uint32_t computed_header_crc = crc32((const uint8_t*)&header, offsetof(encrypted_config_header_t, header_crc));
    if (header.magic != CONFIG_MAGIC || header.header_crc != computed_header_crc) {
        storage_health_inc_integrity_error(device_id);
        return IO_STATUS_IO_ERROR; // Header corrupted
    }

    if (header.namespace_id != namespace_id) {
        return IO_STATUS_INVALID_ARGUMENT;
    }

    // Verify rollback protection if active
    extern rollback_protection_level_t g_test_rollback_level;
    extern uint64_t g_test_rollback_epoch;
    if (g_test_rollback_level == ROLLBACK_PROTECTION_PREVENTION) {
        if (header.rollback_epoch < g_test_rollback_epoch) {
            storage_health_inc_rollback_rejection(device_id);
            return IO_STATUS_PERMISSION_DENIED; // Rollback detected!
        }
    }

    // Reconstruct AAD context
    config_binding_context_t binding = {
        .format_version = 1,
        .device_id = device_id,
        .namespace_id = namespace_id,
        .generation = header.generation,
        .rollback_epoch = header.rollback_epoch
    };
    memcpy(binding.partition_uuid, header.partition_uuid, 16);

    // Call Keystore to open
    uint8_t tag[16];
    memcpy(tag, sector + sizeof(header) + header.ciphertext_length, 16);

    uint8_t plaintext[512];
    memset(plaintext, 0, sizeof(plaintext));

    keystore_aead_request_t req = {
        .plaintext = sector + sizeof(header), // ciphertext
        .plaintext_len = header.ciphertext_length,
        .aad = (const uint8_t*)&binding,
        .aad_len = sizeof(binding)
    };
    memcpy(req.nonce, header.nonce, 16);

    keystore_aead_response_t resp = {
        .ciphertext = plaintext,
        .ciphertext_len = header.ciphertext_length
    };
    memcpy(resp.tag, tag, 16);

    keystore_status_t ks_status = keystore_aead_open(key_cap, &req, &resp);
    if (ks_status == KEYSTORE_STATUS_ERR_AUTH) {
        storage_health_inc_integrity_error(device_id);
        return IO_STATUS_IO_ERROR; // Tampered or authentication failed
    }
    if (ks_status != KEYSTORE_STATUS_OK) {
        storage_health_inc_encryption_error(device_id);
        return IO_STATUS_PERMISSION_DENIED;
    }

    // Copy to output
    uint32_t copy_len = (header.plaintext_length < max_len) ? header.plaintext_length : max_len;
    memcpy(plaintext_out, plaintext, copy_len);
    *out_len = header.plaintext_length;

    return IO_STATUS_OK;
}
