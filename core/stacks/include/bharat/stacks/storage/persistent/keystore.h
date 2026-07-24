#ifndef BHARAT_STACKS_STORAGE_PERSISTENT_KEYSTORE_H
#define BHARAT_STACKS_STORAGE_PERSISTENT_KEYSTORE_H

#include <stdint.h>
#include <stdbool.h>
#include "bharat/io/block.h" // for capability_handle_t

typedef enum {
    KEYSTORE_STATUS_OK = 0,
    KEYSTORE_STATUS_ERR_CAPABILITY,
    KEYSTORE_STATUS_ERR_REVOKED,
    KEYSTORE_STATUS_ERR_AUTH,
    KEYSTORE_STATUS_ERR_INVALID,
    KEYSTORE_STATUS_ERR_PARAMETER
} keystore_status_t;

typedef struct {
    uint16_t format_version;
    uint8_t partition_uuid[16];
    uint32_t device_id;
    uint32_t namespace_id;
    uint64_t generation;
    uint64_t rollback_epoch;
} config_binding_context_t;

typedef struct {
    const uint8_t *plaintext;
    uint32_t plaintext_len;
    const uint8_t *aad;
    uint32_t aad_len;
    uint8_t nonce[16];
} keystore_aead_request_t;

typedef struct {
    uint8_t *ciphertext;
    uint32_t ciphertext_len;
    uint8_t tag[16];
} keystore_aead_response_t;

// Capability check constraints:
typedef struct {
    capability_handle_t cap_handle;
    uint32_t allowed_op; // e.g. 1 for seal, 2 for open
    uint32_t caller_id;
    uint8_t partition_uuid[16];
    bool revoked;
    uint32_t key_id;
} keystore_key_cap_t;

// API functions
keystore_status_t keystore_aead_seal(
    capability_handle_t key_cap,
    const keystore_aead_request_t *request,
    keystore_aead_response_t *response);

keystore_status_t keystore_aead_open(
    capability_handle_t key_cap,
    const keystore_aead_request_t *request,
    keystore_aead_response_t *response);

// Helper to register mock caps for testing
void test_keystore_register_cap(const keystore_key_cap_t *cap);
void test_keystore_revoke_cap(capability_handle_t cap_handle);
void test_keystore_clear(void);

#endif /* BHARAT_STACKS_STORAGE_PERSISTENT_KEYSTORE_H */
