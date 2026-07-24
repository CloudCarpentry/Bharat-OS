#ifndef BHARAT_STACKS_STORAGE_PERSISTENT_CONFIG_STORE_H
#define BHARAT_STACKS_STORAGE_PERSISTENT_CONFIG_STORE_H

#include "bharat/stacks/storage/block.h"
#include "bharat/stacks/storage/persistent/keystore.h"

#define CONFIG_MAGIC 0x43464753U        // "CFGS"
#define CONFIG_SELECTOR_MAGIC 0x53454C54U // "SELT"
#define CONFIG_NONCE_SIZE 16
#define CONFIG_KEY_BINDING_SIZE 32

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t algorithm_id;

    uint64_t generation;
    uint64_t rollback_epoch;

    uint8_t partition_uuid[16];
    uint32_t namespace_id;
    uint32_t plaintext_length;

    uint8_t nonce[CONFIG_NONCE_SIZE];
    uint8_t key_binding_id[CONFIG_KEY_BINDING_SIZE];

    uint32_t ciphertext_length;
    uint32_t header_crc;
} encrypted_config_header_t;

typedef struct {
    uint32_t magic;
    uint32_t active_slot; // 0 for slot A, 1 for slot B
    uint64_t generation;
    uint64_t rollback_epoch;
    uint32_t crc;
} config_selector_t;

// API functions
io_status_t config_store_init(io_device_id_t device_id, const uint8_t partition_uuid[16]);

io_status_t config_store_write(
    io_device_id_t device_id,
    capability_handle_t key_cap,
    uint32_t namespace_id,
    const void *plaintext,
    uint32_t plaintext_len);

io_status_t config_store_read(
    io_device_id_t device_id,
    capability_handle_t key_cap,
    uint32_t namespace_id,
    void *plaintext_out,
    uint32_t max_len,
    uint32_t *out_len);

#endif /* BHARAT_STACKS_STORAGE_PERSISTENT_CONFIG_STORE_H */
